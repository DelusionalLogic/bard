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

static int findBuffer(struct AdvFormatter* formatter, struct Unit* unit, struct RegBuff** buff)
{
	struct Search search = { 0 };
	search.key = unit->name;
	int status = ll_foreach(&formatter->bufferList, searcher, &search);
	if(status == BUFFER_NOTFOUND)
		return BUFFER_NOTFOUND;
	*buff = search.found;
	return BUFFER_FOUND;
}

static int getBuffer(struct AdvFormatter* formatter, struct Unit* unit, struct RegBuff** buff)
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
int advFormatter_format(struct AdvFormatter* formatter, struct Unit* unit)
{
	if(!unit->advancedFormat)
		return 0; //We only do "advanced" formatting

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
