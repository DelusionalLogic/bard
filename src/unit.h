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

#define UNIT_BUFFLEN (1024) //Length of scratch buffer
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

	char* delimiter;

	/* Processing info */
	/* Command buffering */
	unsigned long hash;

	/* Scratch buffer between stages */
	char buffer[1024];
	size_t buffoff; //If we did a partial buffer fill last run

	/* Pipe descriptor if applicable */
	int pipe;
	int writefd;
};

void unit_init(struct Unit* unit);
void unit_kill(struct Unit* unit);

bool unit_setName(struct Unit* unit, const char* name);
bool unit_setType(struct Unit* unit, const enum UnitType type);
bool unit_setCommand(struct Unit* unit, const char* command);
bool unit_setRegex(struct Unit* unit, const char* regex);
bool unit_setAdvFormat(struct Unit* unit, bool advFormat);
bool unit_setFormat(struct Unit* unit, const char* format);
bool unit_setInterval(struct Unit* unit, const int interval);
bool unit_setFonts(struct Unit* unit, const char* key, const char* value);
bool unit_setDelimiter(struct Unit* unit, const char* delimiter);

#endif
