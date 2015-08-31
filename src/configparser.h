#ifndef CONFIGPARSER_H
#define CONFIGPARSER_H

#include <stdbool.h>
#include <iniparser.h>
#include <setjmp.h>
#include "unit.h"

#define	StringConfigEntry(NAME, CB, DEFAULT) { .name = NAME, .type = TYPE_STRING, .set_str = (void (*)(jmp_buf, void*, const char*))CB, .default_string = DEFAULT }
#define	IntConfigEntry(NAME, CB, DEFAULT) { .name = NAME, .type = TYPE_INT, .set_int = (void (*)(jmp_buf, void*, int))CB, .default_int = DEFAULT }
#define	BoolConfigEntry(NAME, CB, DEFAULT) { .name = NAME, .type = TYPE_BOOL, .set_bool = (void (*)(jmp_buf, void*, bool))CB, .default_bool = DEFAULT }
#define	MapConfigEntry(NAME, CB) { .name = NAME, .type = TYPE_MAP, .set_map = (void (*)(jmp_buf, void*, const char*, const char*))CB }
#define EndConfigEntry() { .name = NULL }

enum EntryType {
	TYPE_BOOL,
	TYPE_INT,
	TYPE_STRING,
	TYPE_MAP,
};

#define PTR_SET(NAME, TYPE) void (*NAME)(jmp_buf, void*, TYPE)
struct ConfigParserEntry {
	char* name;
	enum EntryType type;
	union {
		PTR_SET(set_bool, bool);
		PTR_SET(set_int, int);
		PTR_SET(set_str, const char*);
		void (*set_map)(jmp_buf, void*, const char*, const char*);
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

void cp_init(jmp_buf jmpBuf, struct ConfigParser* parser, struct ConfigParserEntry entry[]);
void cp_kill(struct ConfigParser* parser);

void cp_load(jmp_buf jmpBuf, struct ConfigParser* parser, const char* file, void* obj);

#endif
