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

int freePtr(void* elem, void* userdata) {
	char** data = (char**) elem;
	free(*data);
	return 0;
}

int freeUnit(void* elem, void* userdata) {
	struct Unit* unit = (struct Unit*)elem;
	unit_free(unit);
	return 0;
}

int PrintUnit(void* elem, void* userdata)
{
	struct Unit* unit = *(struct Unit**)elem;
	log_write(LEVEL_INFO, unit->name);
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

struct Formatter formatter;
struct Output outputter;
FILE* bar;

int executeUnit(struct Unit* unit)
{
	log_write(LEVEL_INFO, "[%ld] Executing %s (%s, %s)\n", time(NULL), unit->name, unit->command, TypeStr[unit->type]);

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

	/* Don't process things we already have processed */
	unsigned long newHash = hashString((unsigned char*)buff.data);
	if(unit->hash == newHash) {
		vector_delete(&buff);
		return 0;
	}
	unit->hash = newHash;

	/* Format the output for the bar */
	char outBuff[1024];
	formatter_format(&formatter, unit, buff.data, outBuff, 1024);
	vector_delete(&buff);

	color_parseColor(outBuff, 1024);

	strncpy(unit->output, outBuff, 1024);

	return 0;
}

int render() {
	char* out = out_format(&outputter);
	fprintf(bar, "%s\n", out);
	printf("%s\n", out);
	free(out);
	fflush(stdout);
	fflush(bar);
}

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

bool parseType(struct Unit* unit, const char* type)
{
	for(int i = 0; i < UNITTYPE_LENGTH; i++)
	{
		if(!strcasecmp(TypeStr[i], type))
			return unit_setType(unit, (const enum UnitType)i);
	}
	return false;
}

void pipe_handler(int signal) {
	//Handle pipes being ready. We want to tell the conditional that it can read
	log_write(LEVEL_INFO, "Data ready on a pipe");
}

void loadUnits(Vector* units, struct ConfigParser* parser, const char* path) {
	Vector files;
	vector_init(&files, sizeof(char*), 5);
	getFiles(path, &files);
	for(size_t i = 0; i < vector_size(&files); i++)
	{
		log_write(LEVEL_INFO, "Reading config from %s\n", *(char**)vector_get(&files, i));
		char* file = *(char**)vector_get(&files, i);
		struct Unit unit = { 0 };
		unit_init(&unit);

		if(!cp_load(parser, *(char**)vector_get(&files, i), &unit)) {
			log_write(LEVEL_ERROR, "Couldn't parse config %s", file);
			exit(1);
		}

		vector_putBack(units, &unit);
	}
	vector_foreach(&files, freePtr, NULL);
	vector_delete(&files);
}

int main(int argc, char **argv)
{
	struct arguments arguments = {0};
	argp_parse(&argp, argc, argv, 0, 0, &arguments);

	//Set signal handler for the async pipes
	if(signal(SIGIO, pipe_handler) == SIG_ERR) {
		log_write(LEVEL_ERROR, "Could not set signal handler SIGIO");
		exit(1);
	}

	if(arguments.configDir == NULL)
	{
		log_write(LEVEL_ERROR, "Config directory required");
		exit(0);
	}


	log_write(LEVEL_INFO, "Reading configs from %s\n", arguments.configDir);
	struct ConfigParser globalParser;
	struct ConfigParserEntry globalEntries[] = {
		StringConfigEntry("display:separator", conf_setSeparator, ""),
		EndConfigEntry(),
	};
	cp_init(&globalParser, globalEntries);

	struct Conf conf;
	conf_init(&conf);
	
	char* globalPath = pathAppend(arguments.configDir, "bard.conf");
	cp_load(&globalParser, globalPath, &conf);
	free(globalPath);

	Vector left;
	Vector center;
	Vector right;
	vector_init(&left, sizeof(struct Unit), 10);
	vector_init(&center, sizeof(struct Unit), 10);
	vector_init(&right, sizeof(struct Unit), 10);

	struct ConfigParser unitParser;
	struct ConfigParserEntry entries[] = {
		StringConfigEntry("unit:name", unit_setName, NULL),
		StringConfigEntry("unit:type", parseType, (char*)TypeStr[UNIT_POLL]),

		StringConfigEntry("display:command", unit_setCommand, NULL),
		StringConfigEntry("display:regex", unit_setRegex, NULL),
		StringConfigEntry("display:format", unit_setFormat, NULL),
		IntConfigEntry("display:interval", unit_setInterval, 10),
		MapConfigEntry("font", unit_setFonts),
		EndConfigEntry(),
	};
	cp_init(&unitParser, entries);

	char* unitPath = pathAppend(arguments.configDir, "left");
	loadUnits(&left, &unitParser, unitPath);
	free(unitPath);
	unitPath = pathAppend(arguments.configDir, "center");
	loadUnits(&center, &unitParser, unitPath);
	free(unitPath);
	unitPath = pathAppend(arguments.configDir, "right");
	loadUnits(&right, &unitParser, unitPath);
	free(unitPath);

	cp_free(&unitParser);

	Vector fonts;
	vector_init(&fonts, sizeof(char*), 5);
	struct addUnitFontsData fontData = {
		.fonts = &fonts,
	};
	vector_foreach(&left, addUnitFonts, &fontData);
	vector_foreach(&center, addUnitFonts, &fontData);
	vector_foreach(&right, addUnitFonts, &fontData);

	//Input color information
	color_init(arguments.configDir);

	formatter_init(&formatter);

	//Lets load that bar!
	Vector fontSel;
	vector_init(&fontSel, sizeof(char), 64);
	struct fontSelectorData data = {
		.fontSelector = &fontSel,
		.first = true,
	};
	vector_foreach(&fonts, fontSelector, &data);
	vector_putListBack(&fontSel, "\0", 1);
	char lBuff[1024];
	snprintf(lBuff, 1024, "bar -f \"%s\"", fontSel.data);
	printf("%s\n", lBuff);
	vector_delete(&fontSel);

	log_write(LEVEL_INFO, "Starting %s", lBuff);
	bar = popen(lBuff, "w");

	out_init(&outputter, &conf); // Out is called from workmanager_run. It has to be ready before that is called
	out_insert(&outputter, ALIGN_LEFT, &left);
	out_insert(&outputter, ALIGN_CENTER, &center);
	out_insert(&outputter, ALIGN_RIGHT, &right);

	struct WorkManager wm;
	workmanager_init(&wm);
	workmanager_addUnits(&wm, &left);
	workmanager_addUnits(&wm, &center);
	workmanager_addUnits(&wm, &right);

	workmanager_run(&wm, executeUnit, render); //Blocks until the program should exit

	workmanager_free(&wm);

	vector_foreach(&left, freeUnit, NULL);
	vector_delete(&left);

	vector_foreach(&center, freeUnit, NULL);
	vector_delete(&center);

	vector_foreach(&right, freeUnit, NULL);
	vector_delete(&right);

	out_free(&outputter);
	formatter_free(&formatter);

	pclose(bar);

	return 0;
}
