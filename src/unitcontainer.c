#define _GNU_SOURCE
#include "unitcontainer.h"
#include <errno.h>
#include <string.h>
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
	getFiles(jmpBuf, path, &files);
	for(int i = 0; i < vector_size(&files); i++)
	{
		log_write(LEVEL_INFO, "Reading config from %s\n", *(char**)vector_get(jmpBuf, &files, i));
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
