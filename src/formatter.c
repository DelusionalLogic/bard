#include "formatter.h"
#include <regex.h>
#include <stdio.h>
#include <string.h>
#include "logger.h"

struct RegBuff {
	char* key;
	regex_t regex;
};

void formatter_init(struct Formatter* formatter)
{
	ll_init(&formatter->bufferList, sizeof(struct RegBuff));
}

void formatter_free(struct Formatter* formatter)
{
	ll_delete(&formatter->bufferList);
}

struct Search {
	struct RegBuff* found;
	char* key;
};

static bool searcher(void* elem, void* userdata)
{
	struct RegBuff* element = (struct RegBuff*)elem;
	struct Search* search = (struct Search*)userdata;


	if(element->key == search->key || !strcmp(element->key, search->key)) {
		search->found = element;
		return false;
	}
	return true;
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
	log_write(LEVEL_INFO, "Getting buffer\n");
	bool found = findBuffer(formatter, unit, buff);
	if(!found) {
		struct RegBuff newBuff;
		if(regcomp(&newBuff.regex, unit->regex, REG_EXTENDED))
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

bool formatter_format(struct Formatter* formatter, struct Unit* unit, const char* input, char* output, size_t outLen)
{
	struct RegBuff* buffer;
	getBuffer(formatter, unit, &buffer);
//--------------------------THIS IS TUPID I THINK----------------------

	regmatch_t matches[MAX_MATCH];
	log_write(LEVEL_INFO, "%s on %s\n", unit->regex, input);
	if(regexec(&buffer->regex, input, MAX_MATCH, matches, 0))
		log_write(LEVEL_ERROR, "Error matching");

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
	char* outPos = output;
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
		strncpy(outPos, input + match.rm_so, match.rm_eo - match.rm_so);
		outPos += match.rm_eo - match.rm_so;
		curPos += strlen(lookup[index]);
	}
	strncpy(outPos, prevPos, unit->format + formatLen - prevPos);
//---------------------------------------------------------------------
	log_write(LEVEL_INFO, "%s\n", output);
	return true;
}
