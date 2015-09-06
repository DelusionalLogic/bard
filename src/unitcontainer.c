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
#include "fs.h"
#include "unit.h"
#include "configparser.h"

void units_init(struct Units* units) {
	vector_init(&units->left, sizeof(struct Unit), 10);
	vector_init(&units->center, sizeof(struct Unit), 10);
	vector_init(&units->right, sizeof(struct Unit), 10);
}

static int freeUnit(void* elem, void* userdata) {
	struct Unit* unit = (struct Unit*)elem;
	unit_kill(unit);
	return 0;
}

void units_kill(struct Units* units) {
	vector_foreach(&units->left, freeUnit, NULL);
	vector_kill(&units->left);

	vector_foreach(&units->center, freeUnit, NULL);
	vector_kill(&units->center);

	vector_foreach(&units->right, freeUnit, NULL);
	vector_kill(&units->right);
}

static int freePtr(void* elem, void* userdata) {
	char** data = (char**) elem;
	free(*data);
	return 0;
}

static bool parseType(struct Unit* unit, const char* type)
{
	for(int i = 0; i < UNITTYPE_LENGTH; i++)
	{
		if(!strcasecmp(TypeStr[i], type))
			return unit_setType(unit, (const enum UnitType)i);
	}
	return false;
}

static int loadSide(Vector* units, struct ConfigParser* parser, const char* path) {
	Vector files;
	vector_init(&files, sizeof(char*), 5);
	getFiles(path, &files);
	for(int i = 0; i < vector_size(&files); i++)
	{
		log_write(LEVEL_INFO, "Reading config from %s\n", *(char**)vector_get(&files, i));
		char* file = *(char**)vector_get(&files, i);
		struct Unit unit;
		unit_init(&unit);

		int err = cp_load(parser, file, &unit);
		if(err)
			return err;

		vector_putBack(units, &unit);
	}
	vector_foreach(&files, freePtr, NULL);
	vector_kill(&files);
	return 0;
}

int units_load(struct Units* units, char* configDir) {
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

	cp_init(&unitParser, entries);
	
	int err;
	char* unitPath = pathAppend(configDir, "left");
	err = loadSide(&units->left, &unitParser, unitPath);
	if(err)
		return err;
	free(unitPath);
	unitPath = pathAppend(configDir, "center");
	err = loadSide(&units->center, &unitParser, unitPath);
	if(err)
		return err;
	free(unitPath);
	unitPath = pathAppend(configDir, "right");
	err = loadSide(&units->right, &unitParser, unitPath);
	if(err)
		return err;
	free(unitPath);

	cp_kill(&unitParser);
	return 0;
}
