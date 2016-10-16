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
#include "parser.h"
#include "fs.h"
#include "unit.h"
#include "configparser.h"

void units_init(struct Units* units) {
	vector_init(&units->left, sizeof(struct Unit), 10);
	VPROP_THROW("While initializing left unitcontainer");
	vector_init(&units->center, sizeof(struct Unit), 10);
	VPROP_THROW("While initializing center unitcontainer");
	vector_init(&units->right, sizeof(struct Unit), 10);
	VPROP_THROW("While initializing right unitcontainer");
}

static bool freeUnit(void* elem, void* userdata) {
	struct Unit* unit = (struct Unit*)elem;
	unit_kill(unit);
	return true;
}

void units_free(struct Units* units) {
	vector_foreach(&units->left, freeUnit, NULL);
	vector_kill(&units->left);

	vector_foreach(&units->center, freeUnit, NULL);
	vector_kill(&units->center);

	vector_foreach(&units->right, freeUnit, NULL);
	vector_kill(&units->right);
}

static bool freePtr(void* elem, void* userdata) {
	char** data = (char**) elem;
	free(*data);
	return true;
}

static bool parseType(struct Unit* unit, const char* type)
{
	for(int i = 0; i < UNITTYPE_LENGTH; i++)
	{
		if(!strcasecmp(TypeStr[i], type)) {
			unit_setType(unit, (const enum UnitType)i);
			return true;
		}
	}
	return false;
}

static void loadSide(Vector* units, struct ConfigParser* parser, const char* path) {
	Vector files;
	vector_init(&files, sizeof(char*), 5);
	VPROP_THROW("While loading units from %s", path);
	jmp_buf getEx;
	bool dirExists = getFiles(path, &files);
	VPROP_THROW("While loading units from %s", path);
	if(!dirExists) {
		int status = mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
		if(status != 0) {
			int myerrCode = 0;
			switch(errno) {
				case EACCES:
				case EROFS:
					VTHROW_NEW("I don't have permission to created dir %s", path);
				case ENAMETOOLONG:
					VTHROW_NEW("path %s is too long", path);
				case ENOENT:
				case ENOTDIR:
					VTHROW_NEW("%s is not a directory", path);
				case ENOSPC:
					VTHROW_NEW("Something about a lack of space? %s", path);
			}
		}
		getFiles(path, &files);
		VPROP_THROW("While loading units from %s", path);
	}
	for(int i = 0; i < vector_size(&files); i++)
	{
		char* file = *(char**)vector_get(&files, i);
		VPROP_THROW("While loading unit from %s", file);
		log_write(LEVEL_INFO, "Reading config from %s", file);

		struct Unit unit;
		unit_init(&unit);
		VPROP_THROW("While loading unit from %s", file);

		cp_load(parser, file, &unit);
		VPROP_THROW("While loading unit form %s", file);

		vector_putBack(units, &unit);
	}
	vector_foreach(&files, freePtr, NULL);
	vector_kill(&files);
}

void units_load(struct Units* units, char* configDir) {
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
		MapConfigEntry("env", unit_setEnvironment),
		{.name = NULL},
	};

	cp_init(&unitParser, entries);
	VPROP_THROW("While loading units");
	
	char* unitPath = pathAppend(configDir, "left");
	loadSide(&units->left, &unitParser, unitPath);
	VPROP_THROW("While loading left units");
	free(unitPath);

	unitPath = pathAppend(configDir, "center");
	loadSide(&units->center, &unitParser, unitPath);
	VPROP_THROW("While loading center units");
	free(unitPath);

	unitPath = pathAppend(configDir, "right");
	loadSide(&units->right, &unitParser, unitPath);
	VPROP_THROW("While loading right units");
	free(unitPath);

	cp_kill(&unitParser);
}

bool unit_preprocess(void* elem, void* metadata) {
	struct Unit* unit = (struct Unit*)elem;
	log_write(LEVEL_INFO, "Preprocessing unit: %s", unit->name);
	if(!unit->advancedFormat) {
		parser_compileStr(unit->format, &unit->compiledFormat);
		PROP_THROW(false, "While preprocessing %s", unit->name);
	} else {
		char key[12] = "\0"; //Max key length
		char **val;
		JSLF(val, unit->envMap, key);
		while(val != NULL) {
			Vector* vec = malloc(sizeof(Vector));
			parser_compileStr(*val, vec);
			Vector** nval;
			JSLI(nval, unit->compiledEnv, key);
			*nval = vec;
			JSLN(val, unit->envMap, key);
		}
	}
	unit->isPreProcessed = true;
	return true;
}

void units_preprocess(struct Units* units) {
	vector_foreach(&units->left, unit_preprocess, NULL);
	VPROP_THROW("While preprocessing the left side");
	vector_foreach(&units->center, unit_preprocess, NULL);
	VPROP_THROW("While preprocessing the conter");
	vector_foreach(&units->right, unit_preprocess, NULL);
	VPROP_THROW("While preprocessing the right side");
}

struct MatchData {
	char* name;
	struct Unit* result;
};
bool match(void* elem, void* userdata) {
	struct Unit* unit = (struct Unit*)elem;
	struct MatchData* data = (struct MatchData*)userdata;
	if(strcmp(data->name, unit->name) == 0) {
		log_write(LEVEL_INFO, "Found the unit %s", unit->name);
		data->result = unit;
		return false;
	}
	return true;
}

struct Unit* units_find(struct Units* units, char* name) {
	struct MatchData data = {
		.name = name
	};

	bool found = vector_foreach(&units->left, match, &data);
	PROP_THROW(NULL, "While searching the left side");
	if(!found)
		return data.result;

	found = vector_foreach(&units->center, match, &data);
	PROP_THROW(NULL, "While searching the center");
	if(!found)
		return data.result;

	found = vector_foreach(&units->right, match, &data);
	PROP_THROW(NULL, "While searching the right side");
	if(!found)
		return data.result;
	return NULL;
}
