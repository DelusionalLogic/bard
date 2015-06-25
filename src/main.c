#include <config.h>
#include <signal.h>
#include <iniparser.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <argp.h>
#include <stdbool.h>
#include <dirent.h>
#include <time.h>
#include <unistd.h>

#include "unit.h"
#include "formatter.h"
#include "configparser.h"
#include "workmanager.h"
#include "output.h"
#include "logger.h"
#include "vector.h"
#include "linkedlist.h"
#include "conf.h"

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

int fileSort(const void* e1, const void* e2)
{
	return strcmp((char*)e1, (char*)e2);	
}

char* pathAppend(const char* path, const char* path2) {
	size_t pathLen = strlen(path);
	size_t additionalSpace = 0;
	if(path[pathLen-1] != '/')
		additionalSpace = 1;
	char* newPath = malloc(pathLen + strlen(path2) + additionalSpace + 1);
	strcpy(newPath, path);
	if(additionalSpace)
		newPath[pathLen] = '/';
	strcpy(newPath + pathLen + additionalSpace, path2);
	return newPath;
}

//nameList is a vector of string (char*)
void getFiles(const char* path, Vector* nameList)
{
	DIR* dir;
	struct dirent *ent;
	char slash = '/';

	if((dir = opendir(path)) == NULL)
	{
		log_write(LEVEL_ERROR, "Couldn't open directory");
		exit(1);
	}
	while((ent = readdir(dir)) != NULL)
	{
		if(ent->d_name[0] == '.') 
			continue;
		if(ent->d_type != DT_REG && ent->d_type != DT_LNK)
			continue;
		Vector name;
		vector_init(&name, sizeof(char), 100);
		vector_putListBack(&name, path, strlen(path));
		vector_putBack(&name, &slash);
		vector_putListBack(&name, ent->d_name, strlen(ent->d_name)+1);

		vector_putBack(nameList, &name.data);
		//Name is not destroyed because we want to keep the buffer around
	}	
	free(dir);
	vector_qsort(nameList, fileSort);
}

bool freePtr(void* elem, void* userdata) {
	char** data = (char**) elem;
	free(*data);
}

bool freeUnit(void* elem, void* userdata) {
	struct Unit* unit = (struct Unit*)elem;
	unit_free(unit);
}

bool PrintUnit(void* elem, void* userdata)
{
	struct Unit* unit = *(struct Unit**)elem;

	log_write(LEVEL_INFO, unit->name);

	return true;
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

/*#define MAX_MATCH 24
#define LOOKUP_MAX 10
char* getNext(const char* curPos, int* index, const char (*lookups)[LOOKUP_MAX], size_t lookupsLen)
{
	char* curMin = strstr(curPos, lookups[0]);
	*index = 0;
	char* thisPos = NULL;
	for(size_t i = 1; i < lookupsLen; i++)
	{
		thisPos = strstr(curPos, lookups[i]);
		if(thisPos == NULL)
			continue;
		if(curMin == NULL || thisPos < curMin)
		{
			curMin = thisPos;
			*index = i;
		}
	}
	return curMin;
}

void formatStrUnit(struct Unit* unit, const char* input, char* output, size_t outSize)
{
	if(unit->hasRegex)
	{
		if(!unit->isCompiled)
		{
			if(regcomp(&unit->regexComp, unit->regex, REG_EXTENDED))
				log_write(LEVEL_ERROR, "Could not compile regex");
			unit->isCompiled = true;
		}
		size_t maxMatch = MAX_MATCH;
		regmatch_t matches[MAX_MATCH];
		printf("%s on %s\n", unit->regex, input);
		if(regexec(&unit->regexComp, input, maxMatch, matches, 0))
			log_write(LEVEL_ERROR, "Error matching");

		if(!unit->advancedFormat)
		{
			printf("Matching: %s\n", unit->format);
			char lookupmem[MAX_MATCH*LOOKUP_MAX] = {0}; //the string we are looking for. Depending on the MAX_MATCH this might have to be longer
			char (*lookup)[LOOKUP_MAX] = (char (*)[LOOKUP_MAX])lookupmem;
			int numMatches = -1;
			for(int i = 0; i < MAX_MATCH; i++)
			{
				regmatch_t* match = &matches[i];
				if(match->rm_so != -1)
					numMatches = i;

				snprintf(lookup[i], LOOKUP_MAX, "$%d", i); //This should probably be computed at compiletime
				//TODO: Error checking
			}	 
			size_t formatLen = strlen(unit->format);
			const char* curPos = unit->format;
			const char* prevPos = NULL;
			char* outPos = output;
			while(curPos < unit->format + formatLen)
			{
				printf("Looking for next token in %s\n", curPos);
				prevPos = curPos;
				int index = 0;
				curPos = getNext(curPos, &index, lookup, LOOKUP_MAX);

				if(curPos == NULL)
					break;

				strncpy(outPos, prevPos, curPos - prevPos);
				outPos += curPos - prevPos;

				regmatch_t match = matches[index];
				strncpy(outPos, input + match.rm_so, match.rm_eo - match.rm_so);
				outPos += match.rm_eo - match.rm_so;
				curPos++;
			}
		}
	}
}
*/

struct Formatter formatter;
struct Output outputter;

bool executeUnit(struct Unit* unit)
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
	unsigned long newHash = hashString(buff.data);
	if(unit->hash == newHash) {
		vector_delete(&buff);
		return true;
	}
	unit->hash = newHash;

	/* Format the output for the bar */
	char outBuff[1024];
	formatter_format(&formatter, unit, buff.data, outBuff, 1024);
	vector_delete(&buff);

	strncpy(unit->output, outBuff, 1024);

	log_write(LEVEL_INFO, "EXEC %p\n", unit->output);
	out_print(&outputter);

	return true;
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
		struct Unit unit = {0};
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

	formatter_init(&formatter);

	if(arguments.configDir == NULL)
	{
		log_write(LEVEL_ERROR, "Config directory required");
		exit(0);
	}
	log_write(LEVEL_INFO, "Reading configs from %s\n", arguments.configDir);

	struct ConfigParser globalParser;
	struct ConfigParserEntry globalEntries[] = {
		StringConfigEntry("display:separator", conf_setSeparator, ""),
		{.name = NULL},
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
		{.name = NULL},
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
	
	out_init(&outputter, &conf); // Out is called from workmanager_run. It has to be ready before that is called
	out_insert(&outputter, ALIGN_LEFT, &left);
	out_insert(&outputter, ALIGN_CENTER, &center);
	out_insert(&outputter, ALIGN_RIGHT, &right);

	struct WorkManager wm;
	workmanager_init(&wm);
	workmanager_addUnits(&wm, &left);
	workmanager_addUnits(&wm, &center);
	workmanager_addUnits(&wm, &right);

	workmanager_run(&wm, executeUnit); //Blocks until the program should exit

	workmanager_free(&wm);

	vector_foreach(&left, freeUnit, NULL);
	vector_delete(&left);

	vector_foreach(&center, freeUnit, NULL);
	vector_delete(&center);

	vector_foreach(&right, freeUnit, NULL);
	vector_delete(&right);

	out_free(&outputter);
	formatter_free(&formatter);


	return 0;
}
