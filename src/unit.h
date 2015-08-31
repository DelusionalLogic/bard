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
	char buffer[UNIT_BUFFLEN];
	size_t buffoff; //If we did a partial buffer fill last run

	/* Pipe descriptor if applicable */
	int pipe;
	int writefd;
};

void unit_init(jmp_buf jmpBuf, struct Unit* unit);
void unit_kill(struct Unit* unit);

void unit_setName(jmp_buf jmpBuf, struct Unit* unit, const char* name);
void unit_setType(jmp_buf jmpBuf, struct Unit* unit, const enum UnitType type);
void unit_setCommand(jmp_buf jmpBuf, struct Unit* unit, const char* command);
void unit_setRegex(jmp_buf jmpBuf, struct Unit* unit, const char* regex);
void unit_setAdvFormat(jmp_buf jmpBuf, struct Unit* unit, bool advFormat);
void unit_setFormat(jmp_buf jmpBuf, struct Unit* unit, const char* format);
void unit_setInterval(jmp_buf jmpBuf, struct Unit* unit, const int interval);
void unit_setFonts(jmp_buf jmpBuf, struct Unit* unit, const char* key, const char* value);
void unit_setDelimiter(jmp_buf jmpBuf, struct Unit* unit, const char* delimiter);

#endif
