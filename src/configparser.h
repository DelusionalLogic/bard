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
#ifndef CONFIGPARSER_H
#define CONFIGPARSER_H

#include <stdbool.h>
#include <iniparser.h>
#include "unit.h"

#define	StringConfigEntry(NAME, CB, DEFAULT) { .name = NAME, .type = TYPE_STRING, .set_str = (bool (*)(void*, const char*))CB, .default_string = DEFAULT }
#define	IntConfigEntry(NAME, CB, DEFAULT) { .name = NAME, .type = TYPE_INT, .set_int = (bool (*)(void*, int))CB, .default_int = DEFAULT }
#define	BoolConfigEntry(NAME, CB, DEFAULT) { .name = NAME, .type = TYPE_BOOL, .set_bool = (bool (*)(void*, bool))CB, .default_bool = DEFAULT }
#define	MapConfigEntry(NAME, CB) { .name = NAME, .type = TYPE_MAP, .set_map = (bool (*)(void*, const char*, const char*))CB }
#define EndConfigEntry() { .name = NULL }

enum EntryType {
	TYPE_BOOL,
	TYPE_INT,
	TYPE_STRING,
	TYPE_MAP,
};

#define PTR_SET(NAME, TYPE) bool (*NAME)(void*, TYPE)
struct ConfigParserEntry {
	char* name;
	enum EntryType type;
	union {
		PTR_SET(set_bool, bool);
		PTR_SET(set_int, int);
		PTR_SET(set_str, const char*);
		bool (*set_map)(void*, const char*, const char*);
	};
	union {
		bool default_bool;
		int default_int;
		char* default_string;
	};
};
#undef PTR_SET

struct ConfigParser {
	dictionary* conf;
	struct ConfigParserEntry* entries;
};

void cp_init(struct ConfigParser* parser, struct ConfigParserEntry entry[]);
void cp_kill(struct ConfigParser* parser);

int cp_load(struct ConfigParser* parser, const char* file, void* obj);

#endif
