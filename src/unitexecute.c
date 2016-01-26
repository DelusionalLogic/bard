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
#include "unitexecute.h"
#include <string.h>
#include <dirent.h>
#include <stdio.h>
#include <strings.h>
#include <stdlib.h>
#include "logger.h"
#include "myerror.h"

static int min(int a, int b) { return a < b ? a : b; }

struct PipeStage unitexec_getStage() {
	struct PipeStage stage;
	stage.enabled = true;
	stage.obj = NULL;
	stage.create = NULL;
	stage.reload = NULL;
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

bool unitexec_process(jmp_buf jmpBuf, void* obj, struct Unit* unit) {
	if(unit->type != UNIT_POLL)
		return true;
	if(unit->command == NULL)
		return true;
	/* Execute process */
	FILE* f = (FILE*)popen(unit->command, "r");
	Vector buff;
	vector_init(jmpBuf, &buff, sizeof(char), 32);
	size_t readLen;
	char null = '\0';


	/* Read output */
	char chunk[32];
	while((readLen = fread(chunk, 1, 32, f))>0)
		vector_putListBack(jmpBuf, &buff, chunk, readLen);

	if(buff.data[buff.size-1] == '\n')
		buff.data[buff.size-1] = '\0';
	else
		vector_putBack(jmpBuf, &buff, &null);
		
	pclose(f);

	//TODO: Maybe this could be a stage too? make it return some known good, but nonzero value
	/* Don't process things we already have processed */
	unsigned long newHash = hashString(buff.data);
	if(unit->hash == newHash) {
		vector_kill(&buff);
		return false;
	}
	unit->hash = newHash;
	strncpy(unit->buffer, buff.data, min(vector_size(&buff), UNIT_BUFFLEN));
	vector_kill(&buff);
	return true;
}
