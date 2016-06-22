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
#include <setjmp.h>
#include "myerror.h"
#include "logger.h"
#include "parser.h"

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

	unit->lFontKey = 0;
	unit->fontMap = NULL;
	unit->lEnvKey = 0;
	unit->envMap = NULL;
	
	unit->delimiter = NULL;

	unit->hash = 0;

	unit->render = true;

	unit->isPreProcessed = false;

	unit->compiledEnv = NULL;
}

void unit_kill(struct Unit* unit) {
	free(unit->name);
	free(unit->command);
	free(unit->regex);
	free(unit->format);

	// Free fonts
	{
		char key[unit->lFontKey+1];
		struct FontContainer** val;
		JSLF(val, unit->fontMap, key);
		while(val != NULL) {
			free((*val)->font);
			free(*val);
			JSLN(val, unit->fontMap, key);
		}
		Word_t bytes;
		JSLFA(bytes, unit->fontMap);
	}

	// Free env
	{
		char key[unit->lEnvKey+1];
		char** val;
		JSLF(val, unit->envMap, key);
		while(val != NULL) {
			free(*val);
			JSLN(val, unit->envMap, key);
		}
		Word_t bytes;
		JSLFA(bytes, unit->envMap);
	}

	free(unit->delimiter);
}

//Setters {{{
void unit_setName(struct Unit* unit, const char* name) {
	free(unit->name);
	if(name == NULL) {
		unit->name = NULL;
		return;
	}

	size_t nameLen = strlen(name) + 1;
	unit->name = malloc(sizeof(char) * nameLen);
	if(unit->name == NULL) VTHROW_NEW("Failed allocating space for unit name %s", name);
	strcpy(unit->name, name);
}
void unit_setType(struct Unit* unit, const enum UnitType type) {
	unit->type = type;
}
void unit_setCommand(struct Unit* unit, const char* command) {
	free(unit->command);
	if(command == NULL) {
		unit->command = NULL;
		return;
	}

	size_t commandLen = strlen(command) + 1;
	unit->command = malloc(sizeof(char) * commandLen);
	if(unit->command == NULL) VTHROW_NEW("Failed allocating space for unit command %s", command);
	strcpy(unit->command, command);
}
void unit_setRegex(struct Unit* unit, const char* regex) {
	free(unit->regex);
	if(regex == NULL) {
		unit->regex = NULL;
		return;
	}

	Vector str;
		vector_init(&str, sizeof(char), 512);
		//For some insane reason regex depends on all escape chars to already be unescaped before being passsed to it. So here it goes i guess...
		for(int i = 0; regex[i] != '\0'; i++) {
			if(regex[i] == '\\'){
				switch(regex[i+1]) {
					case 'n':
						vector_putBack(&str, "\n");
						VPROP_THROW("While expanding escape characters of regex %s", regex);
						i++;
						break;
					case 't':
						vector_putBack(&str, "\t");
						VPROP_THROW("While expanding escape characters of regex %s", regex);
						i++;
						break;
					case '\\':
						vector_putBack(&str, "\\");
						VPROP_THROW("While expanding escape characters of regex %s", regex);
						i++;
						break;
					default:
						vector_putBack(&str, "\\");
						VPROP_THROW("While expanding escape characters of regex %s", regex);
				}
			} else {
				vector_putBack(&str, &regex[i]);
			}
		}
		vector_putBack(&str, "\0"); //Using a string lets get a char* from a literal
		VPROP_THROW("While adding null terminator to regex");

		unit->hasRegex = true;
		unit->regex = vector_detach(&str);
}
void unit_setAdvFormat(struct Unit* unit, const bool advFormat) {
	unit->advancedFormat = advFormat;
}
void unit_setFormat(struct Unit* unit, const char* format){
	free(unit->format);
	if(format == NULL) {
		unit->format = NULL;
		return;
	}

	size_t formatLen = strlen(format) + 1;
	unit->format = malloc(sizeof(char) * formatLen);
	if(unit->format == NULL) VTHROW_NEW("Failed allocating space for unit format %s", format);
	strcpy(unit->format, format);
}
void unit_setInterval(struct Unit* unit, const int interval) {
	unit->interval = interval;
}
//Add font to fontmap is called once per font
void unit_setFonts(struct Unit* unit, const char* key, const char* value) {
	if(key == NULL || value == NULL)
		VTHROW_NEW("Key or Value for the font was NULL, that's not valid");

	size_t valueLen = strlen(value) + 1;

	struct FontContainer* container = malloc(sizeof(struct FontContainer));
	if(container == NULL) VTHROW_NEW("Failed allocating space for font container for %s", key);

	container->font = malloc(sizeof(char) * valueLen);
	if(container->font == NULL) {
		free(container);
		VTHROW_NEW("Failed allocating space for font container font string for font %s", key);
	}

	strcpy(container->font, value);

	size_t keylen = strlen(key);
	unit->lFontKey = keylen > unit->lFontKey ? keylen : unit->lFontKey;

	struct FontContainer** val;
	JSLI(val, unit->fontMap, (uint8_t*)key);
	*val = container;
}
void unit_setEnvironment(struct Unit* unit, const char* key, const char* value) {
	if(key == NULL || value == NULL)
		VTHROW_NEW("Key or Value for the environment was NULL, that's not valid");

	size_t valueLen = strlen(value);

	size_t keylen = strlen(key);
	unit->lEnvKey = keylen > unit->lEnvKey ? keylen : unit->lEnvKey;

	char** newVal;
	JSLI(newVal, unit->envMap, (uint8_t*)key);
	*newVal = malloc(valueLen * sizeof(char) + 1);
	if(*newVal == NULL) VTHROW_NEW("Failed allocating space for environment value for %s", key);

	strncpy(*newVal, value, valueLen+1);
}

void unit_setDelimiter(struct Unit* unit, const char* delimiter) {
	free(unit->delimiter);
	if(delimiter == NULL) {
		unit->delimiter = NULL;
		return;
	}

	size_t delimiterLen = strlen(delimiter) + 1;
	unit->delimiter = malloc(sizeof(char) * delimiterLen);
	if(unit->delimiter == NULL) VTHROW_NEW("Failed allocating space for unit delimiter %s", delimiter);
	strcpy(unit->delimiter, delimiter);
}
// }}}
