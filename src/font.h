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

#include <Judy.h>
#include "unitcontainer.h"
#include "vector.h"
#include "unit.h"
#include "formatarray.h"

struct FontList {
	Pvoid_t fonts; //Font string to index
	Pvoid_t revFonts; //Index to font string
};

void font_kill(struct FontList* font);

void font_getArray(jmp_buf jmpBuf, struct Unit* unit, struct FormatArray* fmtArray);
void font_createFontList(jmp_buf jmpBuf, struct FontList* font, struct Units* units, char* confPath);
void font_getArg(jmp_buf jmpBuf, struct FontList* font, Vector* args);
#endif
