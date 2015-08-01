#include "font.h"
#include "errno.h"
#include <string.h>
#include <stdio.h>
#include "logger.h"

#define MAXFONTS 10

struct PipeStage font_getStage(){
	struct PipeStage stage;
	stage.error = 0;
	stage.enabled = true;
	stage.obj = calloc(1, sizeof(struct Font));
	if(stage.obj == NULL)
		stage.error = ENOMEM;
	stage.create = (int (*)(void*, char*))font_init;
	stage.addUnits = (int (*)(void*, struct Units*))font_addUnits;
	stage.getArgs = (int (*)(void*, char*, size_t))font_getArgs;
	stage.process = (int (*)(void*, struct Unit*))font_format;
	stage.destroy = (int (*)(void*))font_kill;
	return stage;
}

struct FontCont{
	char* font;
	unsigned long hash;
};

int font_init(struct Font* font, char* configDir) {
	vector_init(&font->fonts, sizeof(struct FontCont), 8);
	return 0;
}

int font_kill(struct Font* font) {
	vector_kill(&font->fonts);
	return 0;
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
static int findFont(void* elem, void* userdata) {
	struct FontCont *cont = (struct FontCont*)elem;
	struct findFontData *data = (struct findFontData*)userdata;

	if(cont->hash == data->needle)
		return -1;
	data->index++;
	return 0;
}

struct addFontsData {
	Vector* fonts;
};
static int addFonts(void* key, void* value, void* userdata) {
	char* k = *(char**)key;
	struct FontContainer* v = *(struct FontContainer**)value;
	struct addFontsData* data = (struct addFontsData*)userdata;

	struct findFontData findData = {
		.needle = hashString(v->font),
		.index = 0,
	};
	if(vector_foreach(data->fonts, findFont, &findData) == -1) {
		v->fontID = findData.index+1; //technically we should look it up and use the fontID, this is a quick hack
		return 0;
	}

	int nextIndex = vector_size(data->fonts);
	v->fontID = nextIndex+1;

	log_write(LEVEL_INFO, "Font %d with selector: %s\n", v->fontID, v->font);
	struct FontCont cont = {
		.font = v->font,
		.hash = hashString(v->font),
	};
	return vector_putBack(data->fonts, &cont);
}

struct addUnitFontsData{
	Vector* fonts;
};
static int addUnitFonts(void* elem, void* userdata) {
	struct Unit* unit = (struct Unit*)elem;
	struct addUnitFontsData* data = (struct addUnitFontsData*)userdata;
	struct addFontsData newData = {
		.fonts = data->fonts,
	};
	return map_foreach(&unit->fontMap, addFonts, &newData);
}

int font_addUnits(struct Font* font, struct Units* units){
	struct addUnitFontsData fontData = {
		.fonts = &font->fonts,
	};
	vector_foreach(&units->left, addUnitFonts, &fontData);
	vector_foreach(&units->center, addUnitFonts, &fontData);
	vector_foreach(&units->right, addUnitFonts, &fontData);
	return 0;
}

struct fontSelectorData{
	Vector* fontSelector;
};
static int fontSelector(void* elem, void* userdata) {	
	char* font = *(char**)elem;
	struct fontSelectorData* data = (struct fontSelectorData*)userdata;

	int fontLen = strlen(font);
	vector_putListBack(data->fontSelector, " -f \"", 5);
	vector_putListBack(data->fontSelector, font, fontLen); 
	vector_putListBack(data->fontSelector, "\"", 1);
	return 0;
}

int font_getArgs(struct Font* font, char* argString, size_t maxLen) {
	Vector fontSel;
	vector_init(&fontSel, sizeof(char), 256);
	struct fontSelectorData data = {
		.fontSelector = &fontSel,
	};
	vector_foreach(&font->fonts, fontSelector, &data);
	vector_putListBack(&fontSel, "\0", 1);
	size_t argLen = strlen(argString);
	if(argLen + vector_size(&fontSel) >= maxLen)
		return ENOMEM;
	strcpy(argString + argLen, fontSel.data);
	vector_kill(&fontSel);
	return 0;
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
static int createLookup(void* key, void* data, void* userdata) {
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
	return 0;
}

int font_format(struct Font* font, struct Unit* unit) {
	Vector newOut;
	vector_init(&newOut, sizeof(char), UNIT_BUFFLEN); 
	
	char lookupmem[MAXFONTS*LOOKUP_MAX] = {0}; //the string we are looking for. Depending on the MAX_MATCH this might have to be longer
	char (*lookup)[LOOKUP_MAX] = (char (*)[LOOKUP_MAX])lookupmem;
	int ids[MAXFONTS]={0};
	struct createLookupData data = {
		.lookup = lookup,
		.ids = &ids,
		.index = 0,
	};
	int err = map_foreach(&unit->fontMap, createLookup, &data);
	if(err != 0) {
		log_write(LEVEL_ERROR, "Failed to construct font selectors");
		return err;
	}
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

		vector_putListBack(&newOut, prevPos, curPos-prevPos);
		char buff[50];
		snprintf(buff, 50, "%d", ids[index]);
		vector_putListBack(&newOut, buff, strlen(buff));
		curPos += strlen(lookup[index]);
	}
	vector_putListBack(&newOut, prevPos, unit->buffer + formatLen - prevPos);
	if(vector_size(&newOut) > UNIT_BUFFLEN) {
		log_write(LEVEL_ERROR, "Output too long");
		return 1;
	}
	strncpy(unit->buffer, newOut.data, vector_size(&newOut));
	vector_kill(&newOut);
	return 0;
}
