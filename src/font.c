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
#include "myerror.h"
#include "fs.h"
#include "logger.h"
#include "configparser.h"

#define MAXFONTS 10

struct FontCont{
	char* font;
	unsigned long hash;
};

struct addFontsData {
	Vector* fonts;
};
static bool addFonts(jmp_buf jmpBuf, void* key, void* value, void* userdata);
int defFont(jmp_buf jmpBuf, struct Font* font, const char* defFont) {
	if(defFont == NULL)
		return true;

	log_write(LEVEL_INFO, "Font: %s", defFont);
	struct FontContainer* contPtr = malloc(sizeof(struct FontContainer));
	contPtr->fontID = 0;
	contPtr->font = malloc(strlen(defFont) * sizeof(char));
	strcpy(contPtr->font, defFont);
	struct addFontsData data = {
		.fonts = &font->fonts,
	};
	addFonts(jmpBuf, /*UNUSED*/ NULL, &contPtr, &data);
	return true;
}

void font_init(jmp_buf jmpBuf, struct Font* font, char* configDir) {
	vector_init(jmpBuf, &font->fonts, sizeof(struct FontCont), 8);

	struct ConfigParser parser;
	struct ConfigParserEntry entry[] = {
		StringConfigEntry("display:font", defFont, NULL),
	};
	cp_init(jmpBuf, &parser, entry);
	char* path = pathAppend(configDir, "bard.conf");
	cp_load(jmpBuf, &parser, path, font);
	free(path);
	cp_kill(&parser);
}

void font_kill(struct Font* font) {
	vector_kill(&font->fonts);
}

static unsigned long hashString(const char *str)
{
	unsigned long hash = 5381;
	int c;

	while ((c = *str++))
		hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

	return hash;
}

struct findFontData {
	unsigned long needle;
	int index;
};
static bool findFont(jmp_buf jmpBuf, void* elem, void* userdata) {
	struct FontCont *cont = (struct FontCont*)elem;
	struct findFontData *data = (struct findFontData*)userdata;

	if(cont->hash == data->needle)
		return false;
	data->index++;
	return true;
}

static bool addFonts(jmp_buf jmpBuf, void* key, void* value, void* userdata) {
	struct FontContainer* v = *(struct FontContainer**)value;
	struct addFontsData* data = (struct addFontsData*)userdata;

	struct findFontData findData = {
		.needle = hashString(v->font),
		.index = 0,
	};
	if(!vector_foreach(jmpBuf, data->fonts, findFont, &findData)) {
		v->fontID = findData.index+1; //technically we should look it up and use the fontID, this is a quick hack
		return true;
	}

	int nextIndex = vector_size(data->fonts);
	v->fontID = nextIndex+1;

	log_write(LEVEL_INFO, "Font %d with selector: %s\n", v->fontID, v->font);
	struct FontCont cont = {
		.font = v->font,
		.hash = hashString(v->font),
	};
	vector_putBack(jmpBuf, data->fonts, &cont);
	return true;
}

struct addUnitFontsData{
	Vector* fonts;
};
static bool addUnitFonts(jmp_buf jmpBuf, void* elem, void* userdata) {
	struct Unit* unit = (struct Unit*)elem;
	struct addUnitFontsData* data = (struct addUnitFontsData*)userdata;
	struct addFontsData newData = {
		.fonts = data->fonts,
	};
	return map_foreach(jmpBuf, &unit->fontMap, addFonts, &newData);
}

void font_addUnits(jmp_buf jmpBuf, struct Font* font, struct Units* units){
	struct addUnitFontsData fontData = {
		.fonts = &font->fonts,
	};
	vector_foreach(jmpBuf, &units->left, addUnitFonts, &fontData);
	vector_foreach(jmpBuf, &units->center, addUnitFonts, &fontData);
	vector_foreach(jmpBuf, &units->right, addUnitFonts, &fontData);
}

struct fontSelectorData{
	Vector* fontSelector;
};
static bool fontSelector(jmp_buf jmpBuf, void* elem, void* userdata) {	
	char* font = *(char**)elem;
	struct fontSelectorData* data = (struct fontSelectorData*)userdata;

	int fontLen = strlen(font);
	vector_putListBack(jmpBuf, data->fontSelector, " -f \"", 5);
	vector_putListBack(jmpBuf, data->fontSelector, font, fontLen); 
	vector_putListBack(jmpBuf, data->fontSelector, "\"", 1);
	return true;
}

void font_getArgs(jmp_buf jmpBuf, struct Font* font, char* argString, size_t maxLen) {
	Vector fontSel;
	vector_init(jmpBuf, &fontSel, sizeof(char), 256);
	struct fontSelectorData data = {
		.fontSelector = &fontSel,
	};
	vector_foreach(jmpBuf, &font->fonts, fontSelector, &data);
	vector_putListBack(jmpBuf, &fontSel, "\0", 1);
	size_t argLen = strlen(argString);
	if(argLen + vector_size(&fontSel) >= maxLen)
		longjmp(jmpBuf, MYERR_NOSPACE);
	strcpy(argString + argLen, fontSel.data);
	vector_kill(&fontSel);
}

bool font_format(jmp_buf jmpBuf, struct Font* font, struct Unit* unit) {
	return true;
}
