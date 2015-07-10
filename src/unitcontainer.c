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
	unit_free(unit);
	return 0;
}

void units_free(struct Units* units) {
	vector_foreach(&units->left, freeUnit, NULL);
	vector_delete(&units->left);

	vector_foreach(&units->center, freeUnit, NULL);
	vector_delete(&units->center);

	vector_foreach(&units->right, freeUnit, NULL);
	vector_delete(&units->right);
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
	vector_delete(&files);
	return 0;
}

int units_load(struct Units* units, char* configDir) {
	struct ConfigParser unitParser;
	struct ConfigParserEntry entries[] = {
		StringConfigEntry("unit:name", unit_setName, NULL),
		StringConfigEntry("unit:type", parseType, (char*)TypeStr[UNIT_POLL]),

		StringConfigEntry("display:command", unit_setCommand, NULL),
		StringConfigEntry("display:regex", unit_setRegex, NULL),
		StringConfigEntry("display:format", unit_setFormat, NULL),
		IntConfigEntry("display:interval", unit_setInterval, 10),
		MapConfigEntry("font", unit_setFonts),
		{.name = NULL},
	};

	cp_init(&unitParser, entries);
	
	char* unitPath = pathAppend(configDir, "left");
	loadSide(&units->left, &unitParser, unitPath);
	free(unitPath);
	unitPath = pathAppend(configDir, "center");
	loadSide(&units->center, &unitParser, unitPath);
	free(unitPath);
	unitPath = pathAppend(configDir, "right");
	loadSide(&units->right, &unitParser, unitPath);
	free(unitPath);

	cp_free(&unitParser);
	return 0;
}
