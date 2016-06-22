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
	Pvoid_t* revFonts;
	int fontIndex;
};
static bool addUnitFonts(void* elem, void* userdata) {
	struct Unit* unit = (struct Unit*)elem;
	struct addUnitFontsData* data = (struct addUnitFontsData*)userdata;

	if(unit->lFontKey == 0)
		return true;

	uint8_t key[unit->lFontKey+1];
	key[0] = '\0';
	struct FontContainer** val;
	JSLF(val, unit->fontMap, key);
	while(val != NULL) {
		uint8_t* key2 = (uint8_t*)(*val)->font;
		int* val2 = NULL;
		JSLG(val2, *data->fonts, key2);
		if(val2 == NULL) {
			JSLI(val2, *data->fonts, key2);
			*val2 = data->fontIndex++;
			char** val3;
			JLI(val3, *data->revFonts, *val2);
			*val3 = (*val)->font;
			log_write(LEVEL_INFO, "Font array key: %d, value: %s", *val2, *val3);
		}
		(*val)->fontID = *val2;
		log_write(LEVEL_INFO, "Font %s (%s) id %d", key, (*val)->font, (*val)->fontID);
		JSLN(val, unit->fontMap, key);
	}
	return true;
}

void font_createFontList(struct FontList* font, struct Units* units, char* confPath) {
	dictionary* dict = iniparser_load(confPath);
	const char* defString = iniparser_getstring(dict, "display:font", NULL);
	if(defString != NULL) {
		char* confStr = malloc((strlen(defString) + 1) * sizeof(char));
		strcpy(confStr, defString);
		int* val; 
		JSLI(val, font->fonts, (uint8_t*)confStr);
		*val = 1;
		char** val2;
		JLI(val2, font->revFonts, *val);
		*val2 = confStr;
	}
	iniparser_freedict(dict);

	struct addUnitFontsData fontData = {
		.fonts = &font->fonts,
		.revFonts = &font->revFonts,
		.fontIndex = 2, //TODO: NOT HARDCODE
	};
	vector_foreach_new(&units->left, addUnitFonts, &fontData);
		VPROP_THROW("While adding left side");
	vector_foreach_new(&units->center, addUnitFonts, &fontData);
		VPROP_THROW("While adding center");
	vector_foreach_new(&units->right, addUnitFonts, &fontData);
		VPROP_THROW("While adding right side");
}

void font_getArray(struct Unit* unit, struct FormatArray* fmtArray) {
	strcpy(fmtArray->name, "font");

	uint8_t key[12] = "\0"; //TODO: HARDCODED MAX LENGTH
	struct FontContainer** val;
	JSLF(val, unit->fontMap, key);
	while(val != NULL) {

		char** val2;
		uint8_t* key2 = key;

		size_t numlen = strlen((char*)key2);
		if(numlen > fmtArray->longestKey)
			fmtArray->longestKey = numlen;

		JSLI(val2, fmtArray->array, key2);
		*val2 = malloc(12 * sizeof(char));
		log_write(LEVEL_INFO, "Font %s id %d", key, (*val)->fontID);
		snprintf(*val2, 12, "%d", (*val)->fontID);
		JSLN(val, unit->fontMap, key);
	}
}

void font_getArg(struct FontList* font, Vector* args) {
	char** val = NULL;
	Word_t key = 0;
	JLF(val, font->revFonts, key);
	while(val != NULL){
		log_write(LEVEL_INFO, "font arg: %d, %s", key, *val);
		vector_putListBack_new(args, " -f \"", 5);
			VPROP_THROW("While adding fonts");
		vector_putListBack_new(args, *val, strlen(*val));
			VPROP_THROW("While adding fonts");
		vector_putListBack_new(args, "\"", 1);
			VPROP_THROW("While adding fonts");
		JLN(val, font->revFonts, key);
	}
	log_write(LEVEL_INFO, "font arg end: %d", key);
}
