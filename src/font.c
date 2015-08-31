#include "font.h"
#include "errno.h"
#include <string.h>
#include <stdio.h>
#include "myerror.h"
#include "fs.h"
#include "logger.h"
#include "configparser.h"

#define MAXFONTS 10

struct PipeStage font_getStage(jmp_buf jmpBuf){
	struct PipeStage stage;
	stage.enabled = true;
	stage.obj = calloc(1, sizeof(struct Font));
	if(stage.obj == NULL)
		longjmp(jmpBuf, MYERR_ALLOCFAIL);
	stage.create = (void (*)(jmp_buf, void*, char*))font_init;
	stage.addUnits = (void (*)(jmp_buf, void*, struct Units*))font_addUnits;
	stage.getArgs = (void (*)(jmp_buf, void*, char*, size_t))font_getArgs;
	stage.process = (bool (*)(jmp_buf, void*, struct Unit*))font_format;
	stage.destroy = (void (*)(void*))font_kill;
	return stage;
}

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

#define LOOKUP_MAX 20
static char* getNext(const char* curPos, int* index, char (*lookups)[LOOKUP_MAX], size_t lookupsLen)
{
	if(lookupsLen == 0)
		return NULL;
	char* curMin = strstr(curPos, lookups[0]);
	*index = 0;
	char* thisPos = NULL;
	for(size_t i = 1; i < lookupsLen; i++)
	{
		//log_write(LEVEL_INFO, "Finding: %s\n", lookups[i]);
		thisPos = strstr(curPos, lookups[i]);
		if(thisPos == NULL)
			continue;
		if(curMin == NULL || thisPos < curMin)
		{
			curMin = thisPos;
			*index = i;
		}
	}
	return curMin;
}

struct createLookupData {
	char (*lookup)[LOOKUP_MAX];
	int (*ids)[MAXFONTS];
	int index;
};
static bool createLookup(jmp_buf jmpBuf, void* key, void* data, void* userdata) {
	char* fontName = *(char**)key;
	struct FontContainer* container = *(struct FontContainer**)data;
	struct createLookupData* udata = (struct createLookupData*)userdata;
	strcpy(udata->lookup[udata->index], "$font[");
	strcpy(udata->lookup[udata->index]+6, fontName);
	int nameLen = strlen(fontName);
	strcpy(udata->lookup[udata->index]+6+nameLen, "]");
	(*udata->ids)[udata->index] = container->fontID;
	fflush(stderr);
	udata->index++;
	return true;
}

bool font_format(jmp_buf jmpBuf, struct Font* font, struct Unit* unit) {
	Vector newOut;
	vector_init(jmpBuf, &newOut, sizeof(char), UNIT_BUFFLEN); 
	
	char lookupmem[MAXFONTS*LOOKUP_MAX] = {0}; //the string we are looking for. Depending on the MAX_MATCH this might have to be longer
	char (*lookup)[LOOKUP_MAX] = (char (*)[LOOKUP_MAX])lookupmem;
	int ids[MAXFONTS]={0};
	struct createLookupData data = {
		.lookup = lookup,
		.ids = &ids,
		.index = 0,
	};
	map_foreach(jmpBuf, &unit->fontMap, createLookup, &data);
	size_t formatLen = strlen(unit->buffer)+1;
	const char* curPos = unit->buffer;
	const char* prevPos = NULL;
	while(curPos < unit->buffer + formatLen)
	{
		prevPos = curPos;
		int index = 0;
		curPos = getNext(curPos, &index, lookup, map_size(&unit->fontMap));

		if(curPos == NULL)
			break;

		vector_putListBack(jmpBuf, &newOut, prevPos, curPos-prevPos);
		char buff[50];
		snprintf(buff, 50, "%d", ids[index]);
		vector_putListBack(jmpBuf, &newOut, buff, strlen(buff));
		curPos += strlen(lookup[index]);
	}
	vector_putListBack(jmpBuf, &newOut, prevPos, unit->buffer + formatLen - prevPos);
	if(vector_size(&newOut) > UNIT_BUFFLEN) {
		log_write(LEVEL_ERROR, "Output too long");
		longjmp(jmpBuf, MYERR_NOSPACE);
	}
	strncpy(unit->buffer, newOut.data, vector_size(&newOut));
	vector_kill(&newOut);
	return true;
}
