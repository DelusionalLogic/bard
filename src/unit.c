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
#include "unit.h"
#include <stdlib.h>
#include <string.h>
#include "logger.h"

bool fontCmp(const void* straw, const void* needle, size_t eSize) {
	char* e1 = *(char**)straw;
	return strcmp(e1, needle) == 0;
}

void unit_init(struct Unit* unit) {
	unit->name = NULL;
	unit->type = UNIT_POLL;
	unit->command = NULL;

	unit->hasRegex = false;
	unit->regex = NULL;

	unit->advancedFormat = false;
	unit->format = NULL;

	unit->interval = 0;

	map_init(&unit->fontMap, sizeof(char*), sizeof(struct FontContainer), fontCmp);
	
	unit->delimiter = NULL;

	unit->hash = 0;
	memset(unit->buffer, 0x00, sizeof(unit->buffer));
	unit->buffoff = 0;

	unit->pipe = -1;
	unit->writefd = -1;
}

void unit_kill(struct Unit* unit) {
	free(unit->name);
	free(unit->command);
	free(unit->regex);
	free(unit->format);

	map_kill(&unit->fontMap);

	free(unit->delimiter);
}

	//Setters
	bool unit_setName(struct Unit* unit, const char* name) {
		free(unit->name);
		if(name == NULL) {
			unit->name = NULL;
			return true;
		}

		size_t nameLen = strlen(name) + 1;
		unit->name = malloc(sizeof(char) * nameLen);
		if(unit->name == NULL) return false;
		strcpy(unit->name, name);
		return true;
	}
	bool unit_setType(struct Unit* unit, const enum UnitType type) {
		unit->type = type;
		return true;
	}
	bool unit_setCommand(struct Unit* unit, const char* command) {
		free(unit->command);
		if(command == NULL) {
			unit->command = NULL;
			return true;
		}

		size_t commandLen = strlen(command) + 1;
		unit->command = malloc(sizeof(char) * commandLen);
		if(unit->command == NULL) return false;
		strcpy(unit->command, command);
		return true;
	}
	bool unit_setRegex(struct Unit* unit, const char* regex) {
		free(unit->regex);
		if(regex == NULL) {
			unit->regex = NULL;
			return true;
		}

		Vector str;
		vector_init(&str, sizeof(char), 512);

		//For some insane reason regex depends on all escape chars to already be unescaped before being passsed to it. So here it goes i guess...
		for(int i = 0; regex[i] != '\0'; i++) {
			if(regex[i] == '\\'){
				switch(regex[i+1]) {
					case 'n':
						vector_putBack(&str, "\n");
						i++;
						break;
					case 't':
						vector_putBack(&str, "\t");
						i++;
						break;
					case '\\':
						vector_putBack(&str, "\\");
						i++;
						break;
					default:
						vector_putBack(&str, "\\");
				}
			} else {
				vector_putBack(&str, &regex[i]);
			}
		}
		vector_putBack(&str, "\0"); //Using a string lets get a char* from a literal

		unit->hasRegex = true;
		unit->regex = str.data;

		return true;
	}
	bool unit_setAdvFormat(struct Unit* unit, const bool advFormat) {
		unit->advancedFormat = advFormat;
		return true;
	}
	bool unit_setFormat(struct Unit* unit, const char* format){
		free(unit->format);
		if(format == NULL) {
			unit->format = NULL;
			return true;
		}

		size_t formatLen = strlen(format) + 1;
		unit->format = malloc(sizeof(char) * formatLen);
		if(unit->format == NULL) return false;
		strcpy(unit->format, format);
		return true;
	}
	bool unit_setInterval(struct Unit* unit, const int interval) {
		unit->interval = interval;
		return true;
	}
	//Add font to fontmap is called once per font
	bool unit_setFonts(struct Unit* unit, const char* key, const char* value) {
		if(key == NULL || value == NULL)
			return true;

		size_t keyLen = strlen(key) + 1;
		size_t valueLen = strlen(value) + 1;

		char* newKey = malloc(sizeof(char) * keyLen);
		if(newKey == NULL) return false;

		struct FontContainer* container = malloc(sizeof(struct FontContainer));
		if(container == NULL) return false;

		container->font = malloc(sizeof(char) * valueLen);
		if(container->font == NULL) return false;

		strcpy(newKey, key);
		strcpy(container->font, value);

		map_put(&unit->fontMap, &newKey, &container);
		return true;
	}

	bool unit_setDelimiter(struct Unit* unit, const char* delimiter) {
		free(unit->delimiter);
		if(delimiter == NULL) {
			unit->delimiter = NULL;
			return true;
		}

		size_t delimiterLen = strlen(delimiter) + 1;
		unit->delimiter = malloc(sizeof(char) * delimiterLen);
		if(unit->delimiter == NULL) return false;
		strcpy(unit->delimiter, delimiter);
		return true;
	}
