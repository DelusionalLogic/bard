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
#define _GNU_SOURCE
#include "advancedformat.h"
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
static bool regBuffFree(jmp_buf jmpBuf, void* elem, void* userdata) {
	struct RegBuff* element = (struct RegBuff*)elem;
	regfree(&element->regex);
	return true;
}

static void advFormatter_free(struct AdvFormatter* formatter) {
	advFormatter_kill(formatter);
	free(formatter);
}
struct PipeStage advFormatter_getStage(jmp_buf jmpBuf) {
	struct PipeStage stage;
	stage.enabled = true;
	stage.obj = calloc(1, sizeof(struct AdvFormatter));
	if(stage.obj == NULL)
		longjmp(jmpBuf, MYERR_ALLOCFAIL);
	stage.create = (void (*)(jmp_buf, void*, char*))advFormatter_init;
	stage.addUnits = NULL;
	stage.getArgs = NULL;
	stage.colorString = NULL;
	stage.process = (bool (*)(jmp_buf, void*, struct Unit*))advFormatter_format;
	stage.destroy = (void (*)(void*))advFormatter_free;
	return stage;
}

void advFormatter_init(jmp_buf jmpBuf, struct AdvFormatter* formatter, char* configDir)
{
	ll_init(jmpBuf, &formatter->bufferList, sizeof(struct RegBuff));
}

void advFormatter_kill(struct AdvFormatter* formatter)
{
	jmp_buf jmpBuf;
	int errCode = setjmp(jmpBuf);
	if(errCode == 0)
		ll_foreach(jmpBuf, &formatter->bufferList, regBuffFree, NULL);
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

static bool findBuffer(jmp_buf jmpBuf, struct AdvFormatter* formatter, struct Unit* unit, struct RegBuff** buff)
{
	struct Search search = { 0 };
	search.key = unit->name;
	if(ll_foreach(jmpBuf, &formatter->bufferList, searcher, &search))
		return false;
	*buff = search.found;
	return true;
}

static struct RegBuff* getBuffer(jmp_buf jmpBuf, struct AdvFormatter* formatter, struct Unit* unit)
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
bool advFormatter_format(jmp_buf jmpBuf, struct AdvFormatter* formatter, struct Unit* unit)
{
	if(!unit->advancedFormat)
		return true; //We only do "advanced" formatting

	//Copy the input from the previous stage
	char buffer[UNIT_BUFFLEN];
	memcpy(buffer, unit->buffer, UNIT_BUFFLEN);
	struct RegBuff* cache;

	regmatch_t matches[MAX_MATCH];
	if(unit->hasRegex) {
		cache = getBuffer(jmpBuf, formatter, unit);
		int err = regexec(&cache->regex, buffer, MAX_MATCH, matches, 0);
		if(err) {
			size_t reqSize = regerror(err, &cache->regex, NULL, 0);
			char *errBuff = malloc(reqSize * sizeof(char));
			regerror(err, &cache->regex, errBuff, reqSize);
			log_write(LEVEL_ERROR, "Error in %s's regex: %s\n", unit->name, errBuff);
			free(errBuff);
			unit->buffer[0] = '\0';
			return false;
		}
	} else {
		matches[0].rm_so = 0;
		matches[0].rm_eo = strlen(buffer);
		matches[1].rm_so = -1;
	}

	Vector rline;
	vector_init(jmpBuf, &rline, sizeof(char), 128);
	vector_putListBack(jmpBuf, &rline, unit->format, strlen(unit->format));
	for(int i = 0; i < MAX_MATCH; i++) {
		if(matches[i].rm_so == -1)
			break;
		vector_putListBack(jmpBuf, &rline, " \'", 2);
		vector_putListBack(jmpBuf, &rline, buffer + matches[i].rm_so, matches[i].rm_eo - matches[i].rm_so);
		vector_putListBack(jmpBuf, &rline, "\'", 1);
	}
	vector_putListBack(jmpBuf, &rline, "\0", 1);

	log_write(LEVEL_INFO, "Executing %s", rline.data);
	FILE* f = (FILE*)popen(rline.data, "r");
	vector_kill(&rline);
	Vector buff;
	vector_init(jmpBuf, &buff, sizeof(char), 32);
	ssize_t readLen;
	char null = '\0';


	/* Read output */
	char chunk[32];
	while((readLen = fread(chunk, 1, 32, f))>0)
		vector_putListBack(jmpBuf, &buff, chunk, readLen);

	if(buff.data[buff.size-1] == '\n')
		buff.data[buff.size-1] = '\0';
	else
		vector_putBack(jmpBuf, &buff, &null);
	strcpy(unit->buffer, buff.data);
	vector_kill(&buff);
	int exitCode = WEXITSTATUS(pclose(f));
	log_write(LEVEL_INFO, "Done: %d", exitCode);
	unit->render = exitCode == 0;
	return true;
}
