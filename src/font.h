#ifndef FONT_H
#define FONT_H

#include "pipestage.h"
#include "unitcontainer.h"
#include "vector.h"
#include "unit.h"

struct Font {
	Vector fonts;
};

struct PipeStage font_getStage();

int font_init(struct Font* font, char* configDir);
int font_kill(struct Font* font);

int font_addUnits(struct Font* font, struct Units* units);

int font_getArgs(struct Font* font, char* argString, size_t maxLen);

int font_format(struct Font* font, struct Unit* unit);

#endif
