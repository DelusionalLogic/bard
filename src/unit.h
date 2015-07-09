#ifndef UNIT_H
#define UNIT_H

#include <stdbool.h>
#include "map.h"

enum UnitType{
	UNIT_POLL,
	UNIT_RUNNING,
};
#define UNITTYPE_LENGTH ((int)UNIT_RUNNING+1)

static const char* const TypeStr[] = {
	"poll",
	"running",
};

struct FontContainer {
	char* font;
	int fontID;
};

struct Unit {
	/* Config File */
	char* name;
	enum UnitType type;
	char* command;

	bool hasRegex;
	char* regex;

	bool advancedFormat;
	char* format;

	int interval;

	struct Map fontMap;

	/* Processing info */
	/* Command buffering */
	unsigned long hash;

	/* Final output */
	char output[1024];
};

void unit_init(struct Unit* unit);
void unit_free(struct Unit* unit);

bool unit_setName(struct Unit* unit, const char* name);
bool unit_setType(struct Unit* unit, const enum UnitType type);
bool unit_setCommand(struct Unit* unit, const char* command);
bool unit_setRegex(struct Unit* unit, const char* regex);
bool unit_setFormat(struct Unit* unit, const char* format);
bool unit_setInterval(struct Unit* unit, const int interval);
bool unit_setFonts(struct Unit* unit, const char* key, const char* value);

#endif
