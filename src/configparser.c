#include "configparser.h"
#include "myerror.h";
//TODO: Rewrite to use integer error codes

void cp_init(jmp_buf jmpBuf, struct ConfigParser* parser, struct ConfigParserEntry entry[]) {
	parser->conf = NULL;
	parser->entries = entry;
}
void cp_kill(struct ConfigParser* parser) {
}

static void set_bool(jmp_buf jmpBuf, struct ConfigParser* parser, struct ConfigParserEntry* entry, void* obj) {
	bool val = iniparser_getboolean(parser->conf, entry->name, entry->default_bool);
	return entry->set_bool(jmpBuf, obj, val);
}

static void set_int(jmp_buf jmpBuf, struct ConfigParser* parser, struct ConfigParserEntry* entry, void* obj) {
	int val = iniparser_getint(parser->conf, entry->name, entry->default_int);
	return entry->set_int(jmpBuf, obj, val);
}

static void set_str(jmp_buf jmpBuf, struct ConfigParser* parser, struct ConfigParserEntry* entry, void* obj) {
	const char* val = iniparser_getstring(parser->conf, entry->name, entry->default_string);
	return entry->set_str(jmpBuf, obj, val);
}

static void set_map(jmp_buf jmpBuf, struct ConfigParser* parser, struct ConfigParserEntry* entry, void* obj) {
	size_t nameLen = strlen(entry->name);
	int secn = iniparser_getsecnkeys(parser->conf, entry->name);
	const char **keys = malloc(secn * sizeof(char*));
	if(keys == NULL)
		longjmp(jmpBuf, MYERR_ALLOCFAIL);
	iniparser_getseckeys(parser->conf, entry->name, keys);

	for(int i = 0; i < secn; i++) {
		const char* val = iniparser_getstring(parser->conf, keys[i], NULL);
		entry->set_map(jmpBuf, obj, keys[i] + nameLen + 1, val);
	}
	free(keys);
}

void cp_load(jmp_buf jmpBuf, struct ConfigParser* parser, const char* file, void* obj) {
	parser->conf = iniparser_load(file);
	if(parser->conf == NULL)
		longjmp(jmpBuf, MYERR_NOTINITIALIZED);
	size_t i = 0;
	while(true)
	{
		struct ConfigParserEntry entry = parser->entries[i];
		if(entry.name == NULL)
			break;
		switch(entry.type) {
			case TYPE_BOOL:
				set_bool(jmpBuf, parser, &entry, obj);
				break;
			case TYPE_INT:
				set_int(jmpBuf, parser, &entry, obj);
				break;
			case TYPE_STRING:
				set_str(jmpBuf, parser, &entry, obj);
				break;
			case TYPE_MAP:
				set_map(jmpBuf, parser, &entry, obj);
				break;
			default:
				//Should never happen
				return;
		}
		i++;
	}
	iniparser_freedict(parser->conf);
}
