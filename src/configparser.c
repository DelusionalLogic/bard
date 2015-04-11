#include "configparser.h"

void cp_init(struct ConfigParser* parser, struct ConfigParserEntry entry[]) {
	parser->conf = NULL;
	parser->entries = entry;
}
void cp_free(struct ConfigParser* parser) {
	if(parser->conf != NULL)
		iniparser_freedict(parser->conf);
}

static bool set_bool(struct ConfigParser* parser, struct ConfigParserEntry* entry, void* obj) {
	bool val = iniparser_getboolean(parser->conf, entry->name, entry->default_bool);
	return entry->set_bool(obj, val);
}

static bool set_int(struct ConfigParser* parser, struct ConfigParserEntry* entry, void* obj) {
	int val = iniparser_getint(parser->conf, entry->name, entry->default_int);
	return entry->set_int(obj, val);
}

static bool set_str(struct ConfigParser* parser, struct ConfigParserEntry* entry, void* obj) {
	char* val = iniparser_getstring(parser->conf, entry->name, entry->default_string);
	return entry->set_str(obj, val);
}

bool cp_load(struct ConfigParser* parser, const char* file, void* obj) {
	parser->conf = iniparser_load(file);
	if(parser->conf == NULL)
		return false;
	bool status = true;
	size_t i = 0;
	while(true)
	{
		struct ConfigParserEntry entry = parser->entries[i];
		if(entry.name == NULL)
			break;
		switch(entry.type) {
			case TYPE_BOOL:
				status = set_bool(parser, &entry, obj);
				break;
			case TYPE_INT:
				status = set_int(parser, &entry, obj);
				break;
			case TYPE_STRING:
				status = set_str(parser, &entry, obj);
				break;
			default:
				//Should never happen
				return false;
		}
		if(!status)
			return false;
		i++;
	}
	return status;
}
