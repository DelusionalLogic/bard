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
#include "font.h"
#include "errno.h"
#include <string.h>
#include <stdio.h>
#include <Judy.h>
#include "myerror.h"
#include "formatarray.h"
#include "fs.h"
#include "logger.h"
#include "configparser.h"

void font_kill(struct FontList* font) {
}

struct addUnitFontsData{
	Pvoid_t* fonts;
	int fontIndex;
};
static bool addUnitFonts(jmp_buf jmpBuf, void* elem, void* userdata) {
	struct Unit* unit = (struct Unit*)elem;
	struct addUnitFontsData* data = (struct addUnitFontsData*)userdata;

	char key[12] = {0};
	struct FontContainer** val;
	JSLF(val, unit->fontMap, key);
	while(val != NULL) {
		char* key2 = (*val)->font;
		int* val2;
		JSLG(val2, *data->fonts, key2);
		if(val2 == NULL) {
			JSLI(val2, *data->fonts, key2);
			*val2 = data->fontIndex++;
		}
		(*val)->fontID = *val2;
		log_write(LEVEL_ERROR, "Font %s id %d", key, (*val)->fontID);
		JSLN(val, unit->fontMap, key);
	}
	return true;
}

void font_createFontList(jmp_buf jmpBuf, struct FontList* font, struct Units* units, char* confPath) {
	dictionary* dict = iniparser_load(confPath);
	const char* defString = iniparser_getstring(dict, "display:font", NULL);
	if(defString != NULL) {
		int* val;
		JSLI(val, font->fonts, defString);
		*val = 0;
	}

	struct addUnitFontsData fontData = {
		.fonts = &font->fonts,
		.fontIndex = 1, //TODO: NOT HARDCODE
	};
	vector_foreach(jmpBuf, &units->left, addUnitFonts, &fontData);
	vector_foreach(jmpBuf, &units->center, addUnitFonts, &fontData);
	vector_foreach(jmpBuf, &units->right, addUnitFonts, &fontData);
}

void font_getArray(jmp_buf jmpBuf, struct Unit* unit, struct FormatArray* fmtArray) {
	strcpy(fmtArray->name, "font");

	char key[12] = "\0"; //TODO: HARDCODED MAX LENGTH
	struct FontContainer** val;
	JSLF(val, unit->fontMap, key);
	while(val != NULL) {

		char** val2;
		char* key2 = key;

		size_t numlen = strlen(key2);
		if(numlen > fmtArray->longestKey)
			fmtArray->longestKey = numlen;

		JSLI(val2, fmtArray->array, key2);
		*val2 = malloc(12 * sizeof(char));
		log_write(LEVEL_INFO, "Font %s id %d", key, (*val)->fontID);
		sprintf(*val2, "%d", (*val)->fontID);
		JSLN(val, unit->fontMap, key);
	}
}

void font_getArg(jmp_buf jmpBuf, struct FontList* font, Vector* args) {
	int** val = 0;
	char key[128] = {0}; //TODO: HARDCODED MAX FONT SELECTOR LENGTH
	JSLF(val, font->fonts, key);
	while(val != NULL){
		vector_putListBack(jmpBuf, args, " -f \"", 5);
		vector_putListBack(jmpBuf, args, &key, strlen(key));
		vector_putListBack(jmpBuf, args, "\"", 1);
		JSLN(val, font->fonts, key);
	}
}
