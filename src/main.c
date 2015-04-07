#include <config.h>
#include <regex.h>
#include <iniparser.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <argp.h>
#include <stdbool.h>
#include <dirent.h>
#include <time.h>

#include "logger.h"
#include "vector.h"
#include "linkedlist.h"

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
		printf("%s\n", name.data);

		vector_putBack(nameList, &name.data);
		//Name is not destroyed because we want to keep the buffer around
	}	
	vector_qsort(nameList, fileSort);
}

enum UnitType{
	UNIT_POLL,
	UNIT_RUNNING,
};

#define UNITTYPE_LENGTH ((int)UNIT_RUNNING+1)

static const char* const TypeStr[] = {
	"poll",
	"running",
};

#define NAME_MAX 255
#define COMMAND_MAX 1024
#define REGEX_MAX 1024
#define FORMAT_MAX 255
#define LASTOUT_MAX 1024
struct Unit {
	char name[NAME_MAX];
	enum UnitType type;
	char command[COMMAND_MAX];
	unsigned long lastCmdOut;

	bool hasRegex;
	char regex[REGEX_MAX];
	bool isCompiled;
	regex_t regexComp;

	bool advancedFormat;
	char format[FORMAT_MAX];

	int interval;
	time_t nextRun;
	char lastOut[LASTOUT_MAX];
};

struct UnitSearch {
	struct Unit* unit;
	int slot;
};

bool FindSlot(void* elem, void* userdata)
{
	struct Unit* unit = *(struct Unit**)elem;
	struct UnitSearch* s = (struct UnitSearch*)userdata;
	if(unit->nextRun > s->unit->nextRun)
		return false;

	s->slot++;
	return true;
}

bool PrintUnit(void* elem, void* userdata)
{
	struct Unit* unit = *(struct Unit**)elem;

	log_write(LEVEL_INFO, unit->name);

	return true;
}

void insertWork(LinkedList* workList, struct Unit* unit)
{
	struct UnitSearch search;
	search.unit = unit;
	search.slot = 0;
	ll_foreach(workList, FindSlot, &search);
	ll_insert(workList, search.slot, &search.unit);
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

#define MAX_MATCH 24
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

void executeUnit(struct Unit* unit)
{
	printf("[%ld] %s (%s, %s)\n", time(NULL), unit->name, unit->command, TypeStr[unit->type]);

	FILE* f = (FILE*)popen(unit->command, "r");
	char buff[1024];
	fgets(buff, 1024, f);
	pclose(f);
	unsigned long cmdOut = hashString((unsigned char*)buff);
	if(cmdOut == unit->lastCmdOut)
		return;
	unit->lastCmdOut = cmdOut;

	char outBuff[1024];

	formatStrUnit(unit, buff, outBuff, 1024);

	printf("%s", outBuff);
}

int main(int argc, char **argv)
{
	struct arguments arguments = {0};
	argp_parse(&argp, argc, argv, 0, 0, &arguments);
	if(arguments.configDir == NULL)
	{
		log_write(LEVEL_ERROR, "Config directory required");
		exit(0);
	}
	log_write(LEVEL_INFO, arguments.configDir);

	Vector files;
	vector_init(&files, sizeof(char*), 5);
	getFiles(arguments.configDir, &files);

	Vector units;
	vector_init(&units, sizeof(struct Unit), 10);

	time_t curTime = time(NULL); 

	for(size_t i = 0; i < vector_size(&files); i++)
	{
		printf("%s\n", *(char**)vector_get(&files, i));
		dictionary *conf = iniparser_load(*(char**)vector_get(&files, i));
		struct Unit unit = {0};

		char* name = iniparser_getstring(conf, "unit:name", "UNDEFINED");
		if(strlen(name) >= NAME_MAX)
		{
			log_write(LEVEL_ERROR, "Unit name length greater than max");
			continue;
		}
		strcpy(unit.name, name);

		char* type = iniparser_getstring(conf, "unit:type", "poll");
		for(int i = 0; i < UNITTYPE_LENGTH; i++)
		{
			if(!strcasecmp(TypeStr[i], type))
				unit.type = i;
		}	

		char* command = iniparser_getstring(conf, "display:command", NULL);
		if(strlen(command) >= COMMAND_MAX)
		{
			log_write(LEVEL_ERROR, "Unit command length greater than max");
			continue;
		}
		strcpy(unit.command, command);

		char* regex = iniparser_getstring(conf, "display:regex", NULL);
		if(regex == NULL)
		{
			log_write(LEVEL_INFO, "No regex set");
			unit.hasRegex = false;
		}else if(strlen(regex) >= REGEX_MAX){
			log_write(LEVEL_ERROR, "Regex length greather than max");	
			continue;
		}else{
			unit.hasRegex = true;
			strcpy(unit.regex, regex);
		}

		char* format = iniparser_getstring(conf, "display:format", NULL);
		if(format == NULL)
		{
			log_write(LEVEL_INFO, "No format string set");
			if(unit.hasRegex)
			{
				log_write(LEVEL_ERROR, "Unit has regex but no information on how to format it");
				continue;
			}
		}else if(strlen(format) >= FORMAT_MAX){
			log_write(LEVEL_ERROR, "Format length greather than max");
			continue;
		}else{
			strcpy(unit.format, format);
		}

		unit.advancedFormat = iniparser_getboolean(conf, "display:advFormat", false);

		unit.interval = iniparser_getint(conf, "display:interval", 10);

		unit.nextRun = curTime + unit.interval;

		vector_putBack(&units, &unit);
	}

	vector_delete(&files);

	LinkedList work;
	ll_init(&work, sizeof(struct Unit*));

	for(size_t i = 0; i < vector_size(&units); i++)
	{
		struct Unit* unit = (struct Unit*)vector_get(&units, i);
		insertWork(&work, unit);
	}

	ll_foreach(&work, PrintUnit, NULL);

	{
		while(ll_size(&work) != 0)
		{
			time_t curTime = time(NULL);

			struct Unit* next = *(struct Unit**)ll_get(&work, 0);
			sleep(next->nextRun-curTime);
			curTime = time(NULL);

			ll_remove(&work, 0);
			executeUnit(next);
			next->nextRun = curTime + next->interval;
			insertWork(&work, next);
		}
	}

	ll_delete(&work);

	vector_delete(&units);


	return 0;
}
