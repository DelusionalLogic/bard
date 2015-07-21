#include "formatter.h"
#include <regex.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "logger.h"

struct RegBuff {
	char* key;
	regex_t regex;
};
int regBuffFree(void* elem, void* userdata) {
	struct RegBuff* element = (struct RegBuff*)elem;
	regfree(&element->regex);
	return 0;
}

static void formatter_free(struct Formatter* formatter) {
	formatter_kill(formatter);
	free(formatter);
}
struct PipeStage formatter_getStage() {
	struct PipeStage stage;
	stage.enabled = true;
	stage.obj = calloc(1, sizeof(struct Formatter));
	if(stage.obj == NULL)
		stage.error = ENOMEM;
	stage.create = (int (*)(void*, char*))formatter_init;
	stage.addUnits = NULL;
	stage.getArgs = NULL;
	stage.process = (int (*)(void*, struct Unit*))formatter_format;
	stage.destroy = (int (*)(void*))formatter_free;
	return stage;
}

int formatter_init(struct Formatter* formatter, char* configDir)
{
	ll_init(&formatter->bufferList, sizeof(struct RegBuff));
	return 0;
}

int formatter_kill(struct Formatter* formatter)
{
	ll_foreach(&formatter->bufferList, regBuffFree, NULL);
	ll_kill(&formatter->bufferList);
	return 0;
}

struct Search {
	struct RegBuff* found;
	char* key;
};

#define SEARCH_DONE 1
#define SEARCH_CONT 0
static int searcher(void* elem, void* userdata)
{
	struct RegBuff* element = (struct RegBuff*)elem;
	struct Search* search = (struct Search*)userdata;


	if(element->key == search->key || !strcmp(element->key, search->key)) {
		search->found = element;
		return SEARCH_DONE;
	}
	return SEARCH_CONT;
}

static bool findBuffer(struct Formatter* formatter, struct Unit* unit, struct RegBuff** buff)
{
	struct Search search = { 0 };
	search.key = unit->name;
	ll_foreach(&formatter->bufferList, searcher, &search);
	*buff = search.found;
	return buff == NULL;
}

static bool getBuffer(struct Formatter* formatter, struct Unit* unit, struct RegBuff** buff)
{
	bool found = findBuffer(formatter, unit, buff);
	if(!found) {
		struct RegBuff newBuff = {0};
		newBuff.key = NULL;
		if(regcomp(&newBuff.regex, unit->regex, REG_EXTENDED | REG_NEWLINE))
			log_write(LEVEL_ERROR, "Could not compile regex for %s\n", unit->name);
		newBuff.key = unit->name;
		*buff = ll_insert(&formatter->bufferList, ll_size(&formatter->bufferList), &newBuff);
	}
	return found;
}

#define MAX_MATCH 24
#define LOOKUP_MAX 10
static char* getNext(const char* curPos, int* index, char (*lookups)[LOOKUP_MAX], size_t lookupsLen)
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

int formatter_format(struct Formatter* formatter, struct Unit* unit)
{
	if(unit->advancedFormat)
		return 0; //We only do "simple" formatting
	//
	//Copy the input from the previous stage
	char buffer[UNIT_BUFFLEN];
	memcpy(buffer, unit->buffer, UNIT_BUFFLEN);
	struct RegBuff* cache;
	getBuffer(formatter, unit, &cache);

	regmatch_t matches[MAX_MATCH];
	int err = regexec(&cache->regex, buffer, MAX_MATCH, matches, 0);
	if(err) {
		size_t reqSize = regerror(err, &cache->regex, NULL, 0);
		char *errBuff = malloc(reqSize * sizeof(char));
		regerror(err, &cache->regex, errBuff, reqSize);
		log_write(LEVEL_ERROR, "Error in %s's regex: %s\n", unit->name, errBuff);
		free(errBuff);
		return 2;
	}

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
	size_t formatLen = strlen(unit->format)+1;
	const char* curPos = unit->format;
	const char* prevPos = NULL;
	char* outPos = unit->buffer;
	while(curPos < unit->format + formatLen)
	{
		prevPos = curPos;
		int index = 0;
		curPos = getNext(curPos, &index, lookup, LOOKUP_MAX);

		if(curPos == NULL)
			break;

		strncpy(outPos, prevPos, curPos - prevPos);
		outPos += curPos - prevPos;

		regmatch_t match = matches[index];
		memcpy(outPos, buffer + match.rm_so, match.rm_eo - match.rm_so);
		outPos += match.rm_eo - match.rm_so;
		curPos += strlen(lookup[index]);
	}
	strncpy(outPos, prevPos, unit->format + formatLen - prevPos);
	//---------------------------------------------------------------------
	return 0;
}
