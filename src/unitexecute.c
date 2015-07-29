#define _GNU_SOURCE
#include "unitexecute.h"
#include <string.h>
#include <dirent.h>
#include <stdio.h>
#include <strings.h>
#include <stdlib.h>
#include "logger.h"

static int min(int a, int b) { return a < b ? a : b; }

struct PipeStage unitexec_getStage() {
	struct PipeStage stage;
	stage.enabled = true;
	stage.obj = NULL;
	stage.create = NULL;
	stage.addUnits = NULL;
	stage.getArgs = NULL;
	stage.colorString = NULL;
	stage.process = unitexec_process;
	stage.destroy = NULL;
	return stage;
}

static unsigned long hashString(char *str)
{
	unsigned long hash = 5381;
	int c;

	while ((c = *str++))
		hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

	return hash;
}

int unitexec_process(void* obj, struct Unit* unit) {
	if(unit->type != UNIT_POLL)
		return 0;
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

	//TODO: Maybe this could be a stage too? make it return some known good, but nonzero value
	/* Don't process things we already have processed */
	unsigned long newHash = hashString(buff.data);
	if(unit->hash == newHash) {
		vector_kill(&buff);
		return -1;
	}
	unit->hash = newHash;
	strncpy(unit->buffer, buff.data, min(vector_size(&buff), UNIT_BUFFLEN));
	vector_kill(&buff);
	return 0;
}
