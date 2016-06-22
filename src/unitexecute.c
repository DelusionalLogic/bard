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
#include <assert.h>
#include "logger.h"
#include "myerror.h"

static int min(int a, int b) { return a < b ? a : b; }

static unsigned long hashString(char *str)
{
	unsigned long hash = 5381;
	int c;

	while ((c = *str++))
		hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

	return hash;
}

void unitexec_execUnit(struct Unit* unit, char** out) {
	assert(unit->type == UNIT_POLL);
	assert(unit->command != NULL);

	/* Execute process */
	FILE* f = (FILE*)popen(unit->command, "r");
	if(f == NULL)
		VTHROW_NEW("Failed running command for unit %s", unit->name);

	Vector buff;
	vector_init(&buff, sizeof(char), 512);
	VPROP_THROW("While allocating the final output buffer");
	size_t readLen;
	char null = '\0';


	/* Read output */
	char chunk[1024];
	while((readLen = fread(chunk, 1, 1024, f)) > 0) {
		vector_putListBack(&buff, chunk, readLen);
		VPROP_THROW("While appending to the final output buffer, length: %d", readLen);
	}

	if(buff.size > 0 && buff.data[buff.size-1] == '\n') {
		buff.data[buff.size-1] = '\0';
	} else {
		vector_putBack(&buff, &null);
		VPROP_THROW("While appeding the null terminator to final output buffer");
	}

	pclose(f);

	//TODO: Maybe this could be a stage too? make it return some known good, but nonzero value
	/* Don't process things we already have processed */
	unsigned long newHash = hashString(buff.data);
	if(unit->hash == newHash) {
		vector_kill(&buff);
		*out = NULL;
		return;
	}
	unit->hash = newHash;
	*out = vector_detach(&buff);
}
