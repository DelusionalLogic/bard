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
