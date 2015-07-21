#define _GNU_SOURCE
#include "advancedformat.h"
#include <regex.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "logger.h"

struct RegBuff {
	char* key;
	regex_t regex;
};
static int regBuffFree(void* elem, void* userdata) {
	struct RegBuff* element = (struct RegBuff*)elem;
	regfree(&element->regex);
	return 0;
}

static void advFormatter_free(struct AdvFormatter* formatter) {
	advFormatter_kill(formatter);
	free(formatter);
}
struct PipeStage advFormatter_getStage() {
	struct PipeStage stage;
	stage.enabled = true;
	stage.obj = calloc(1, sizeof(struct AdvFormatter));
	if(stage.obj == NULL)
		stage.error = ENOMEM;
	stage.create = (int (*)(void*, char*))advFormatter_init;
	stage.addUnits = NULL;
	stage.getArgs = NULL;
	stage.process = (int (*)(void*, struct Unit*))advFormatter_format;
	stage.destroy = (int (*)(void*))advFormatter_free;
	return stage;
}

int advFormatter_init(struct AdvFormatter* formatter, char* configDir)
{
	ll_init(&formatter->bufferList, sizeof(struct RegBuff));
	return 0;
}

int advFormatter_kill(struct AdvFormatter* formatter)
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

static bool findBuffer(struct AdvFormatter* formatter, struct Unit* unit, struct RegBuff** buff)
{
	struct Search search = { 0 };
	search.key = unit->name;
	ll_foreach(&formatter->bufferList, searcher, &search);
	*buff = search.found;
	return buff == NULL;
}

static bool getBuffer(struct AdvFormatter* formatter, struct Unit* unit, struct RegBuff** buff)
{
	bool found = findBuffer(formatter, unit, buff);
	if(!found) {
		struct RegBuff newBuff;
		if(regcomp(&newBuff.regex, unit->regex, REG_EXTENDED | REG_NEWLINE))
			log_write(LEVEL_ERROR, "Could not compile regex for %s\n", unit->name);
		newBuff.key = unit->name;
		*buff = ll_insert(&formatter->bufferList, ll_size(&formatter->bufferList), &newBuff);
	}
	return found;
}

#define MAX_MATCH 24
int advFormatter_format(struct AdvFormatter* formatter, struct Unit* unit)
{
	if(!unit->advancedFormat)
		return 0; //We only do "advanced" formatting
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

	Vector rline;
	vector_init(&rline, sizeof(char), 128);
	vector_putListBack(&rline, unit->format, strlen(unit->format));
	for(int i = 0; i < MAX_MATCH; i++) {
		if(matches[i].rm_so == -1)
			break;
		vector_putListBack(&rline, " \"", 2);
		vector_putListBack(&rline, buffer + matches[i].rm_so, matches[i].rm_eo - matches[i].rm_so);
		vector_putListBack(&rline, "\"", 1);
	}
	vector_putListBack(&rline, "\0", 1);

	log_write(LEVEL_INFO, "Executing %s", rline.data);
	FILE* f = (FILE*)popen(rline.data, "r");
	vector_kill(&rline);
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
	strcpy(unit->buffer, buff.data);
	vector_kill(&buff);
	pclose(f);
	return 0;
}
