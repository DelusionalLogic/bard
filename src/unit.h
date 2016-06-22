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
#ifndef UNIT_H
#define UNIT_H

#include <stdbool.h>
#include <Judy.h>
#include <setjmp.h>
#include "vector.h"

enum UnitType{
	UNIT_STATIC,
	UNIT_POLL,
	UNIT_RUNNING,
};
#define UNITTYPE_LENGTH ((int)UNIT_RUNNING+1)

static const char* const TypeStr[] = {
	"static",
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

	size_t lFontKey;
	Pvoid_t fontMap; // str -> FontContainer
	size_t lEnvKey;
	Pvoid_t envMap; // str -> str

	char* delimiter;

	/* Processing info */
	/* Command buffering */
	unsigned long hash;

	/* True if we should be rendered */
	bool render;

	bool isPreProcessed;
	/* Only available after preprocess */
	Vector compiledFormat; //Vector of Node
	Pvoid_t compiledEnv; //str -> Vector of Node
};

void unit_init(struct Unit* unit);
void unit_kill(struct Unit* unit);

void unit_setName(struct Unit* unit, const char* name);
void unit_setType(struct Unit* unit, const enum UnitType type);
void unit_setCommand(struct Unit* unit, const char* command);
void unit_setRegex(struct Unit* unit, const char* regex);
void unit_setAdvFormat(struct Unit* unit, bool advFormat);
void unit_setFormat(struct Unit* unit, const char* format);
void unit_setInterval(struct Unit* unit, const int interval);
void unit_setFonts(struct Unit* unit, const char* key, const char* value);
void unit_setEnvironment(struct Unit* unit, const char* key, const char* value);
void unit_setDelimiter(struct Unit* unit, const char* delimiter);

#endif
