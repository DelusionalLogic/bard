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
	stage.error = 0;
	stage.enabled = true;
	stage.obj = calloc(1, sizeof(struct Formatter));
	if(stage.obj == NULL)
		stage.error = ENOMEM;
	stage.create = (int (*)(void*, char*))formatter_init;
	stage.addUnits = NULL;
	stage.getArgs = NULL;
	stage.colorString = NULL;
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

#define BUFFER_FOUND 1
#define BUFFER_NOTFOUND 0
#define BUFFER_CREATED -1
static int searcher(void* elem, void* userdata)
{
	struct RegBuff* element = (struct RegBuff*)elem;
	struct Search* search = (struct Search*)userdata;


	if(element->key == search->key || !strcmp(element->key, search->key)) {
		search->found = element;
		return BUFFER_FOUND;
	}
	return BUFFER_NOTFOUND;
}

static int findBuffer(struct Formatter* formatter, struct Unit* unit, struct RegBuff** buff)
{
	struct Search search = { 0 };
	search.key = unit->name;
	int status = ll_foreach(&formatter->bufferList, searcher, &search);
	if(status == BUFFER_NOTFOUND)
		return BUFFER_NOTFOUND;
	*buff = search.found;
	return BUFFER_FOUND;
}

static int getBuffer(struct Formatter* formatter, struct Unit* unit, struct RegBuff** buff)
{
	int found = findBuffer(formatter, unit, buff);
	if(found == 0) {
		struct RegBuff newBuff = {0};
		newBuff.key = NULL;
		int err = regcomp(&newBuff.regex, unit->regex, REG_EXTENDED | REG_NEWLINE);
		if(err){
			size_t reqSize = regerror(err, &newBuff.regex, NULL, 0);
			char *errBuff = malloc(reqSize * sizeof(char));
			regerror(err, &newBuff.regex, errBuff, reqSize);
			log_write(LEVEL_ERROR, "Could not compile regex for %s: %s", unit->name, errBuff);
			free(errBuff);
			return BUFFER_NOTFOUND;
		}
		newBuff.key = unit->name;
		*buff = ll_insert(&formatter->bufferList, ll_size(&formatter->bufferList), &newBuff);
		return BUFFER_CREATED;
	}
	return BUFFER_FOUND;
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

	//Copy the input from the previous stage
	char buffer[UNIT_BUFFLEN];
	memcpy(buffer, unit->buffer, UNIT_BUFFLEN);
	struct RegBuff* cache;

	regmatch_t matches[MAX_MATCH];
	if(unit->hasRegex) {
		if(getBuffer(formatter, unit, &cache) == BUFFER_NOTFOUND)
			return 1;
		int err = regexec(&cache->regex, buffer, MAX_MATCH, matches, 0);
		if(err) {
			size_t reqSize = regerror(err, &cache->regex, NULL, 0);
			char *errBuff = malloc(reqSize * sizeof(char));
			regerror(err, &cache->regex, errBuff, reqSize);
			log_write(LEVEL_ERROR, "Error in %s's regex: %s\n", unit->name, errBuff);
			free(errBuff);
			return 2;
		}
	} else {
		matches[0].rm_so = 0;
		matches[0].rm_eo = strlen(buffer)+1;
		matches[1].rm_so = -1;
	}

	char lookupmem[MAX_MATCH*LOOKUP_MAX] = {0}; //the string we are looking for. Depending on the MAX_MATCH this might have to be longer
	char (*lookup)[LOOKUP_MAX] = (char (*)[LOOKUP_MAX])lookupmem;
	int numMatches = -1;
	for(int i = 0; i < MAX_MATCH; i++)
	{
		regmatch_t* match = &matches[i];
		if(match->rm_so != -1)
			numMatches = i+1;

		int written = snprintf(lookup[i], LOOKUP_MAX, "$%d", i+1); //This should probably be computed at compiletime
		if(written < 0)
			return 1; //Should never happen
		if(written > LOOKUP_MAX) {
			log_write(LEVEL_FATAL, "FORMAT SEARCH BUFFER TOO SHORT TO HOLD KEYWORD");
			return 1;
		}
	}	 
	size_t formatLen = strlen(unit->format)+1;
	const char* curPos = unit->format;
	const char* prevPos = NULL;
	char* outPos = unit->buffer;
	while(curPos < unit->format + formatLen)
	{
		prevPos = curPos;
		int index = 0;
		curPos = getNext(curPos, &index, lookup, numMatches);

		if(curPos == NULL)
			break;

		strncpy(outPos, prevPos, curPos - prevPos);
		outPos += curPos - prevPos;
		log_write(LEVEL_INFO, "Found: %d", index);

		regmatch_t match = matches[index];
		memcpy(outPos, buffer + match.rm_so, match.rm_eo - match.rm_so);
		outPos += match.rm_eo - match.rm_so;
		curPos += strlen(lookup[index]);
	}
	strncpy(outPos, prevPos, unit->format + formatLen - prevPos);
	//---------------------------------------------------------------------
	return 0;
}
