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
#include "unitcontainer.h"
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "myerror.h"
#include "fs.h"
#include "unit.h"
#include "configparser.h"

void units_init(jmp_buf jmpBuf, struct Units* units) {
	vector_init(jmpBuf, &units->left, sizeof(struct Unit), 10);
	vector_init(jmpBuf, &units->center, sizeof(struct Unit), 10);
	vector_init(jmpBuf, &units->right, sizeof(struct Unit), 10);
}

static bool freeUnit(jmp_buf jmpBuf, void* elem, void* userdata) {
	struct Unit* unit = (struct Unit*)elem;
	unit_kill(unit);
	return true;
}

void units_kill(jmp_buf jmpBuf, struct Units* units) {
	vector_foreach(jmpBuf, &units->left, freeUnit, NULL);
	vector_kill(&units->left);

	vector_foreach(jmpBuf, &units->center, freeUnit, NULL);
	vector_kill(&units->center);

	vector_foreach(jmpBuf, &units->right, freeUnit, NULL);
	vector_kill(&units->right);
}

static bool freePtr(jmp_buf jmpBuf, void* elem, void* userdata) {
	char** data = (char**) elem;
	free(*data);
	return true;
}

static bool parseType(jmp_buf jmpBuf, struct Unit* unit, const char* type)
{
	for(int i = 0; i < UNITTYPE_LENGTH; i++)
	{
		if(!strcasecmp(TypeStr[i], type)) {
			unit_setType(jmpBuf, unit, (const enum UnitType)i);
			return true;
		}
	}
	return false;
}

static void loadSide(jmp_buf jmpBuf, Vector* units, struct ConfigParser* parser, const char* path) {
	Vector files;
	vector_init(jmpBuf, &files, sizeof(char*), 5);
	jmp_buf getEx;
	int errCode = setjmp(getEx);
	if(errCode == 0) {
		getFiles(getEx, path, &files);
	} else if(errCode == MYERR_NODIR) {
		int status = mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
		if(status != 0) {
			int myerrCode = 0;
			switch(errno) {
				case EACCES:
				case EROFS:
					myerrCode = MYERR_NOPERM;
					break;
				case ENAMETOOLONG:
					myerrCode = MYERR_PATHLENGTH;
					break;
				case ENOENT:
				case ENOTDIR:
					myerrCode = MYERR_NODIR;
					break;
				case ENOSPC:
					myerrCode = MYERR_NOSPACE;
					break;
			}
			longjmp(jmpBuf, myerrCode);
		}
		getFiles(jmpBuf, path, &files);
	} else {
		longjmp(jmpBuf, errCode);
	}
	for(int i = 0; i < vector_size(&files); i++)
	{
		log_write(LEVEL_INFO, "Reading config from %s", *(char**)vector_get(jmpBuf, &files, i));
		char* file = *(char**)vector_get(jmpBuf, &files, i);
		struct Unit unit;
		unit_init(jmpBuf, &unit);

		cp_load(jmpBuf, parser, file, &unit);

		vector_putBack(jmpBuf, units, &unit);
	}
	vector_foreach(jmpBuf, &files, freePtr, NULL);
	vector_kill(&files);
}

void units_load(jmp_buf jmpBuf, struct Units* units, char* configDir) {
	struct ConfigParser unitParser;
	struct ConfigParserEntry entries[] = {
		StringConfigEntry("unit:name", unit_setName, NULL),
		StringConfigEntry("unit:type", parseType, (char*)TypeStr[UNIT_POLL]),

		StringConfigEntry("process:delimiter", unit_setDelimiter, "\n"),

		StringConfigEntry("display:command", unit_setCommand, NULL),
		StringConfigEntry("display:regex", unit_setRegex, NULL),
		BoolConfigEntry("display:advformat", unit_setAdvFormat, false),
		StringConfigEntry("display:format", unit_setFormat, NULL),
		IntConfigEntry("display:interval", unit_setInterval, 10),
		MapConfigEntry("font", unit_setFonts),
		{.name = NULL},
	};

	cp_init(jmpBuf, &unitParser, entries);
	
	char* unitPath = pathAppend(configDir, "left");
	loadSide(jmpBuf, &units->left, &unitParser, unitPath);
	free(unitPath);

	unitPath = pathAppend(configDir, "center");
	loadSide(jmpBuf, &units->center, &unitParser, unitPath);
	free(unitPath);

	unitPath = pathAppend(configDir, "right");
	loadSide(jmpBuf, &units->right, &unitParser, unitPath);
	free(unitPath);

	cp_kill(&unitParser);
}
