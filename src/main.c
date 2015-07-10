#define _GNU_SOURCE
#include <config.h>
#include <signal.h>
#include <iniparser.h>
#include <stdio.h>
#include <strings.h>
#include <stdlib.h>
#include <argp.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>

#include "pipestage.h"
#include "unitcontainer.h"
#include "map.h"
#include "unit.h"
#include "formatter.h"
#include "configparser.h"
#include "workmanager.h"
#include "output.h"
#include "logger.h"
#include "vector.h"
#include "linkedlist.h"
#include "conf.h"
#include "color.h"
#include "fs.h"

static int min(int a, int b) { return a < b ? a : b; }

const char* argp_program_version = PACKAGE_STRING;
const char* argp_program_bug_address = PACKAGE_BUGREPORT;

static char doc[] = "bard";
static char args_doc[] = "";
static struct argp_option options[] = {
	{ "config", 'c', "config", 0, "Configuration directory."},
	{ 0 }
};

struct arguments
{
	char* configDir;
};

static error_t parse_opt(int key, char* arg, struct argp_state *state)
{
	struct arguments *args = state->input;

	switch(key)
	{
		case 'c':
			args->configDir = arg;
			break;
		default:
			return ARGP_ERR_UNKNOWN;
	}
	return 0;
}

static struct argp argp = {options, parse_opt, args_doc, doc, 0, 0, 0};

#define NUM_STAGES 10
struct PipeStage pipeline[NUM_STAGES];

int freePtr(void* elem, void* userdata) {
	char** data = (char**) elem;
	free(*data);
	return 0;
}

unsigned long hashString(unsigned char *str)
{
	unsigned long hash = 5381;
	int c;

	while ((c = *str++))
		hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

	return hash;
}

struct PatternMatch{
	size_t startPos;
	size_t endPos;
};

struct Output outputter;
FILE* bar;

int executeUnit(struct Unit* unit)
{
	log_write(LEVEL_INFO, "[%ld] Executing %s (%s, %s)\n", time(NULL), unit->name, unit->command, TypeStr[unit->type]);

	//TODO: Cutout and replace with a pipeline stage
	/* Execute process */
	FILE* f = (FILE*)popen(unit->command, "r");
	Vector buff;
	vector_init(&buff, sizeof(char), 32);
	ssize_t readLen;
	char null = '\0';


	/* Read output */
	char chunk[32];
	while((readLen = fread(chunk, 1, 32, f))>0)
		vector_putListBack(&buff, chunk, readLen);

	if(buff.data[buff.size-1] == '\n')
		buff.data[buff.size-1] = '\0';
	else
		vector_putBack(&buff, &null);
		
	pclose(f);

	//TODO: Maybe this could be a stage too? make it return some known good, but nonzero value
	/* Don't process things we already have processed */
	unsigned long newHash = hashString((unsigned char*)buff.data);
	if(unit->hash == newHash) {
		vector_kill(&buff);
		return 0;
	}
	unit->hash = newHash;
	strncpy(unit->buffer, buff.data, min(vector_size(&buff), UNIT_BUFFLEN));
	vector_kill(&buff);

	/* Format the output for the bar */
	for(int i = 0; i < NUM_STAGES; i++) {
		struct PipeStage stage = pipeline[i];
		if(stage.obj == NULL)
			break;
		if(stage.process != NULL)
			stage.process(stage.obj, unit);
	}

	return 0;
}

int render() {
	//What to do about this? It can't be pipelined because then i might run more than once
	//per sleep
	char* out = out_format(&outputter, NULL);
	fprintf(bar, "%s\n", out);
	printf("%s\n", out);
	free(out);
	fflush(stdout);
	fflush(bar);
}

//TODO: Move into own pipeline step
struct addFontsData {
	Vector* fonts;
};
int addFonts(void* key, void* value, void* userdata) {
	char* k = *(char**)key;
	struct FontContainer* v = *(struct FontContainer**)value;
	struct addFontsData* data = (struct addFontsData*)userdata;

	int nextIndex = vector_size(data->fonts);
	v->fontID = nextIndex+1;

	log_write(LEVEL_INFO, "Font %d with selector: %s\n", v->fontID, v->font);
	return vector_putBack(data->fonts, &v->font);
}

struct addUnitFontsData{
	Vector* fonts;
};
int addUnitFonts(void* elem, void* userdata) {
	struct Unit* unit = (struct Unit*)elem;
	struct addUnitFontsData* data = (struct addUnitFontsData*)userdata;
	struct addFontsData newData = {
		.fonts = data->fonts,
	};
	return map_foreach(&unit->fontMap, addFonts, &newData);
}

struct fontSelectorData{
	Vector* fontSelector;
	bool first;
};
int fontSelector(void* elem, void* userdata) {	
	char* font = *(char**)elem;
	struct fontSelectorData* data = (struct fontSelectorData*)userdata;

	if(!data->first)
		vector_putListBack(data->fontSelector, ",", 1);
	data->first = false;
	int fontLen = strlen(font);
	vector_putListBack(data->fontSelector, font, fontLen); 
}
//ENDTODO

void pipe_handler(int signal) {
	//Handle pipes being ready. We want to tell the conditional that it can read
	log_write(LEVEL_INFO, "Data ready on a pipe");
}

int main(int argc, char **argv)
{
	struct arguments arguments = {0};
	argp_parse(&argp, argc, argv, 0, 0, &arguments);

	//This shouldn't be useful anymore
	//Set signal handler for the async pipes
	if(signal(SIGIO, pipe_handler) == SIG_ERR) {
		log_write(LEVEL_ERROR, "Could not set signal handler SIGIO");
		exit(1);
	}

	if(arguments.configDir == NULL)
	{
		log_write(LEVEL_ERROR, "Config directory required\n");
		exit(0);
	}

	//Setup pipeline
	pipeline[0] = formatter_getStage();
	pipeline[1] = color_getStage();

	log_write(LEVEL_INFO, "Reading configs from %s\n", arguments.configDir);
	struct ConfigParser globalParser;
	struct ConfigParserEntry globalEntries[] = {
		StringConfigEntry("display:separator", conf_setSeparator, ""),
		StringConfigEntry("bar:geometry", conf_setGeometry, NULL),
		EndConfigEntry(),
	};
	cp_init(&globalParser, globalEntries);

	struct Conf conf;
	conf_init(&conf);
	
	char* globalPath = pathAppend(arguments.configDir, "bard.conf");
	cp_load(&globalParser, globalPath, &conf);
	free(globalPath);

	struct Units units;
	units_init(&units);

	units_load(&units, arguments.configDir);

	//TODO; Should be part of the font pipeline step
	Vector fonts;
	vector_init(&fonts, sizeof(char*), 5);
	struct addUnitFontsData fontData = {
		.fonts = &fonts,
	};
	vector_foreach(&units.left, addUnitFonts, &fontData);
	vector_foreach(&units.center, addUnitFonts, &fontData);
	vector_foreach(&units.right, addUnitFonts, &fontData);

	//Initialize all stages in pipeline (This is where they load the configuration)
	for(int i = 0; i < NUM_STAGES; i++) {
		struct PipeStage stage = pipeline[i];
		if(stage.obj == NULL)
			break;
		stage.create(stage.obj, arguments.configDir);
	}

	//TODO: Where the hell does this belong?
	//Lets load that bar!
	Vector fontSel;
	vector_init(&fontSel, sizeof(char), 64);
	struct fontSelectorData data = {
		.fontSelector = &fontSel,
		.first = true,
	};
	vector_foreach(&fonts, fontSelector, &data);
	vector_putListBack(&fontSel, "\0", 1);
	char lBuff[2048];
	int written = snprintf(lBuff, 2048, "bar -f \"%s\" %s%s", 
			fontSel.data,
			conf.geometry != NULL ? "-g " : "",
			conf.geometry);
	if(written >= 2048 || written < 0) {
		log_write(LEVEL_ERROR, "Couldn't prepare the bar launch string (probably too long)\n");
		exit(1);
	}
	vector_kill(&fontSel);

	log_write(LEVEL_INFO, "Starting %s\n", lBuff);
	bar = popen(lBuff, "w");
	
	for(int i = 0; i < NUM_STAGES; i++) {
		struct PipeStage stage = pipeline[i];
		if(stage.obj == NULL)
			break;
		if(stage.addUnits != NULL)
			stage.addUnits(stage.obj, &units);
	}
	//Now lets run the units in a loop and write to bar
	out_init(&outputter, &conf); // Out is called from workmanager_run. It has to be ready before that is called
	out_addUnits(&outputter, &units);

	struct WorkManager wm;
	workmanager_init(&wm);
	workmanager_addUnits(&wm, &units);

	workmanager_run(&wm, executeUnit, render); //Blocks until the program should exit

	workmanager_kill(&wm);


	out_kill(&outputter);

	for(int i = 0; i < NUM_STAGES; i++) {
		struct PipeStage stage = pipeline[i];
		if(stage.obj == NULL)
			break;
		if(stage.destroy != NULL)
			stage.destroy(stage.obj);
	}

	pclose(bar);

	return 0;
}
