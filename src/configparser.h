#ifndef CONFIGPARSER_H
#define CONFIGPARSER_H

#include <stdbool.h>
#include <iniparser.h>
#include "unit.h"

#define PTR_SET(NAME, TYPE) bool (*NAME)(void*, TYPE)

enum EntryType {
	TYPE_BOOL,
	TYPE_INT,
	TYPE_STRING,
};

struct ConfigParserEntry {
	char* name;
	enum EntryType type;
	union {
		PTR_SET(set_bool, bool);
		PTR_SET(set_int, int);
		PTR_SET(set_str, char*);
	};
	union {
		bool default_bool;
		int default_int;
		char* default_string;
	};
};

struct ConfigParser {
	dictionary* conf;
	struct ConfigParserEntry* entries;
};

void cp_init(struct ConfigParser* parser, struct ConfigParserEntry entry[]);
void cp_free(struct ConfigParser* parser);

bool cp_load(struct ConfigParser* parser, const char* file, void* obj);

#endif
