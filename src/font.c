#include "font.h"
#include "errno.h"
#include <string.h>
#include "logger.h"

struct PipeStage font_getStage(){
	struct PipeStage stage;
	stage.obj = calloc(1, sizeof(struct Font));
	if(stage.obj == NULL)
		stage.error = ENOMEM;
	stage.create = (int (*)(void*, char*))font_init;
	stage.addUnits = (int (*)(void*, struct Units*))font_addUnits;
	stage.getArgs = (int (*)(void*, char*, size_t))font_getArgs;
	stage.process = NULL;
	stage.destroy = (int (*)(void*))font_kill;
	return stage;
}

int font_init(struct Font* font, char* configDir) {
	vector_init(&font->fonts, sizeof(char*), 8);
	return 0;
}

int font_kill(struct Font* font) {
	vector_kill(&font->fonts);
	return 0;
}

struct addFontsData {
	Vector* fonts;
};
static int addFonts(void* key, void* value, void* userdata) {
	char* k = *(char**)key;
	struct FontContainer* v = *(struct FontContainer**)value;
	struct addFontsData* data = (struct addFontsData*)userdata;

	int nextIndex = vector_size(data->fonts);
	v->fontID = nextIndex+1;

	log_write(LEVEL_INFO, "Font %d with selector: %s\n", v->fontID, v->font);
	return vector_putBack(data->fonts, &v->font);
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
}

struct fontSelectorData{
	Vector* fontSelector;
	bool first;
};
static int fontSelector(void* elem, void* userdata) {	
	char* font = *(char**)elem;
	struct fontSelectorData* data = (struct fontSelectorData*)userdata;

	if(!data->first)
		vector_putListBack(data->fontSelector, ",", 1);
	data->first = false;
	int fontLen = strlen(font);
	vector_putListBack(data->fontSelector, font, fontLen); 
	return 0;
}

int font_getArgs(struct Font* font, char* argString, size_t maxLen) {
	Vector fontSel;
	vector_init(&fontSel, sizeof(char), 256);
	struct fontSelectorData data = {
		.fontSelector = &fontSel,
		.first = true,
	};
	vector_putListBack(&fontSel, " -f \"", 5);
	vector_foreach(&font->fonts, fontSelector, &data);
	vector_putListBack(&fontSel, "\"", 1);
	vector_putListBack(&fontSel, "\0", 1);
	size_t argLen = strlen(argString);
	if(argLen + vector_size(&fontSel) >= maxLen)
		return ENOMEM;
	strcpy(argString + argLen, fontSel.data);
	vector_kill(&fontSel);
	return 0;
}
