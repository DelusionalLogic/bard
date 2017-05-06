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
#include "configparser.h"
#include "myerror.h"
#include "logger.h"
//TODO: Rewrite to use integer error codes

void cp_init(struct ConfigParser* parser, struct ConfigParserEntry entry[]) {
	parser->conf = NULL;
	parser->entries = entry;
}
void cp_kill(struct ConfigParser* parser) {
}

static void set_bool(struct ConfigParser* parser, struct ConfigParserEntry* entry, void* obj) {
	bool val = iniparser_getboolean(parser->conf, entry->name, entry->default_bool);
	return entry->set_bool(obj, val);
}

static void set_int(struct ConfigParser* parser, struct ConfigParserEntry* entry, void* obj) {
	int val = iniparser_getint(parser->conf, entry->name, entry->default_int);
	return entry->set_int(obj, val);
}

static void set_str(struct ConfigParser* parser, struct ConfigParserEntry* entry, void* obj) {
	const char* val = iniparser_getstring(parser->conf, entry->name, entry->default_string);
	return entry->set_str(obj, val);
}

static void set_map(struct ConfigParser* parser, struct ConfigParserEntry* entry, void* obj) {
	size_t nameLen = strlen(entry->name);
	int secn = iniparser_getsecnkeys(parser->conf, entry->name);
	if(secn == 0)
		return;
	const char **keys = malloc(secn * sizeof(char*));
	if(keys == NULL)
		VTHROW_NEW("Failed allocating space for the map keys for %s", entry->name);
	iniparser_getseckeys(parser->conf, entry->name, keys);

	for(int i = 0; i < secn; i++) {
		const char* val = iniparser_getstring(parser->conf, keys[i], NULL);
		entry->set_map(obj, keys[i] + nameLen + 1, val);
	}
	free(keys);
}

void cp_load(struct ConfigParser* parser, const char* file, void* obj) {
	parser->conf = iniparser_load(file);
	if(parser->conf == NULL)
		VTHROW_NEW("ConfigParser wasn't initialized for loading %s", file);
	size_t i = 0;
	while(true)
	{
		struct ConfigParserEntry entry = parser->entries[i];
		if(entry.name == NULL)
			break;
		switch(entry.type) {
			case TYPE_BOOL:
				set_bool(parser, &entry, obj);
				break;
			case TYPE_INT:
				set_int(parser, &entry, obj);
				break;
			case TYPE_STRING:
				set_str(parser, &entry, obj);
				break;
			case TYPE_MAP:
				set_map(parser, &entry, obj);
				break;
			default:
				//Should never happen
				VTHROW_NEW("Unknown config type for %s, type was %d", entry.name, entry.type);
				break;
		}
		i++;
	}
	iniparser_freedict(parser->conf);
}
