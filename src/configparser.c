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
//TODO: Rewrite to use integer error codes

void cp_init(struct ConfigParser* parser, struct ConfigParserEntry entry[]) {
	parser->conf = NULL;
	parser->entries = entry;
}
void cp_kill(struct ConfigParser* parser) {
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
	const char* val = iniparser_getstring(parser->conf, entry->name, entry->default_string);
	return entry->set_str(obj, val);
}

static bool set_map(struct ConfigParser* parser, struct ConfigParserEntry* entry, void* obj) {
	size_t nameLen = strlen(entry->name);
	int secn = iniparser_getsecnkeys(parser->conf, entry->name);
	const char **keys = malloc(secn * sizeof(char*));
	iniparser_getseckeys(parser->conf, entry->name, keys);

	bool status = true;
	for(int i = 0; i < secn; i++) {
		const char* val = iniparser_getstring(parser->conf, keys[i], NULL);
		if(!entry->set_map(obj, keys[i] + nameLen + 1, val))
			status = false;
	}
	free(keys);
	return status;
}

int cp_load(struct ConfigParser* parser, const char* file, void* obj) {
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
			case TYPE_MAP:
				status = set_map(parser, &entry, obj);
				break;
			default:
				//Should never happen
				return 1;
		}
		if(!status)
			break;
		i++;
	}
	iniparser_freedict(parser->conf);
	if(!status)
		return 1;
	return 0;
}
