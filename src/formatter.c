//Copyright (C) 2015 Jesper Jensen
//    This file is part of bard.
//
//    bard is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    bard is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with bard.  If not, see <http://www.gnu.org/licenses/>.
#include "formatter.h"
#include <regex.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "myerror.h"
#include "logger.h"

struct RegBuff {
	char* key;
	regex_t regex;
};
bool regBuffFree(jmp_buf jmpBuf, void* elem, void* userdata) {
	struct RegBuff* element = (struct RegBuff*)elem;
	regfree(&element->regex);
	return true;
}

static void formatter_free(struct Formatter* formatter) {
	formatter_kill(formatter);
	free(formatter);
}
struct PipeStage formatter_getStage(jmp_buf jmpBuf) {
	struct PipeStage stage;
	stage.enabled = true;
	stage.obj = calloc(1, sizeof(struct Formatter));
	if(stage.obj == NULL)
		longjmp(jmpBuf, MYERR_ALLOCFAIL);
	stage.create = (void (*)(jmp_buf, void*, char*))formatter_init;
	stage.addUnits = NULL;
	stage.getArgs = NULL;
	stage.colorString = NULL;
	stage.process = (bool (*)(jmp_buf, void*, struct Unit*))formatter_format;
	stage.destroy = (void (*)(void*))formatter_free;
	return stage;
}

void formatter_init(jmp_buf jmpBuf, struct Formatter* formatter, char* configDir)
{
	ll_init(jmpBuf, &formatter->bufferList, sizeof(struct RegBuff));
}

void formatter_kill(struct Formatter* formatter)
{
	ll_foreach(NULL, &formatter->bufferList, regBuffFree, NULL);
	ll_kill(&formatter->bufferList);
}

struct Search {
	struct RegBuff* found;
	char* key;
};
static bool searcher(jmp_buf jmpBuf, void* elem, void* userdata)
{
	struct RegBuff* element = (struct RegBuff*)elem;
	struct Search* search = (struct Search*)userdata;


	if(element->key == search->key || !strcmp(element->key, search->key)) {
		search->found = element;
		return false;
	}
	return true;
}

static bool findBuffer(jmp_buf jmpBuf, struct Formatter* formatter, struct Unit* unit, struct RegBuff** buff)
{
	struct Search search = { 0 };
	search.key = unit->name;
	if(ll_foreach(jmpBuf, &formatter->bufferList, searcher, &search))
		return false;
	*buff = search.found;
	return true;
}

static struct RegBuff* getBuffer(jmp_buf jmpBuf, struct Formatter* formatter, struct Unit* unit)
{
	jmp_buf getEx;
	int errCode = setjmp(getEx);
	if(errCode == 0) {
		struct RegBuff* buff;
		if(!findBuffer(getEx, formatter, unit, &buff)) {
			struct RegBuff newBuff = {0};
			newBuff.key = NULL;
			int err = regcomp(&newBuff.regex, unit->regex, REG_EXTENDED | REG_NEWLINE);
			if(err){
				size_t reqSize = regerror(err, &newBuff.regex, NULL, 0);
				char *errBuff = malloc(reqSize * sizeof(char));
				regerror(err, &newBuff.regex, errBuff, reqSize);
				log_write(LEVEL_ERROR, "Could not compile regex for %s: %s", unit->name, errBuff);
				free(errBuff);
				longjmp(jmpBuf, err);
			}
			newBuff.key = unit->name;
			jmp_buf insEx;
			int errCode = setjmp(insEx);
			if(errCode == 0) {
				buff = ll_insert(insEx, &formatter->bufferList, ll_size(&formatter->bufferList), &newBuff);
			} else {
				log_write(LEVEL_ERROR, "Failed inserting the regex into the cache");
				regfree(&newBuff.regex);
				longjmp(jmpBuf, errCode);
			}
		}
		return buff;
	} else {
		log_write(LEVEL_ERROR, "Error while looking for the regex in cache");
		longjmp(jmpBuf, errCode);
	}
}

#define MAX_MATCH 24
#define LOOKUP_MAX 10
static char* getNext(jmp_buf jmpBuf, const char* curPos, int* index, char (*lookups)[LOOKUP_MAX], size_t lookupsLen)
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

bool formatter_format(jmp_buf jmpBuf, struct Formatter* formatter, struct Unit* unit)
{
	if(unit->advancedFormat)
		return true;

	//Copy the input from the previous stage
	char buffer[UNIT_BUFFLEN];
	memcpy(buffer, unit->buffer, UNIT_BUFFLEN);
	struct RegBuff* cache;

	regmatch_t matches[MAX_MATCH];
	if(unit->hasRegex) {
		cache = getBuffer(jmpBuf, formatter, unit);
		int err = regexec(&cache->regex, buffer, MAX_MATCH, matches, 0);
		if(err == REG_NOMATCH) {
			log_write(LEVEL_INFO, "No match in %s", unit->name);
			matches[0].rm_so = 0;
			matches[0].rm_eo = strlen(buffer)+1;
			matches[1].rm_so = -1;
		}else if(err) {
			size_t reqSize = regerror(err, &cache->regex, NULL, 0);
			char *errBuff = malloc(reqSize * sizeof(char));
			regerror(err, &cache->regex, errBuff, reqSize);
			log_write(LEVEL_ERROR, "Error in %s's regex: %s", unit->name, errBuff);
			free(errBuff);
			unit->buffer[0] = '\0';
			longjmp(jmpBuf, err);
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
		if(match->rm_so == -1)
			break;

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
		curPos = getNext(jmpBuf, curPos, &index, lookup, numMatches);

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
	return true;
}
