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

struct PipeStage font_getStage();

int font_init(struct Font* font, char* configDir);
int font_kill(struct Font* font);

int font_addUnits(struct Font* font, struct Units* units);

int font_getArgs(struct Font* font, char* argString, size_t maxLen);

int font_format(struct Font* font, struct Unit* unit);

#endif
