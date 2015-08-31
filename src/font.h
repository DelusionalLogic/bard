#ifndef FONT_H
#define FONT_H

#include "pipestage.h"
#include "unitcontainer.h"
#include "vector.h"
#include "unit.h"

struct Font {
	Vector fonts;
};

struct PipeStage font_getStage(jmp_buf jmpBuf);

void font_init(jmp_buf jmpBuf, struct Font* font, char* configDir);
void font_kill(struct Font* font);

void font_addUnits(jmp_buf jmpBuf, struct Font* font, struct Units* units);

void font_getArgs(jmp_buf jmpBuf, struct Font* font, char* argString, size_t maxLen);

bool font_format(jmp_buf jmpBuf, struct Font* font, struct Unit* unit);

#endif
