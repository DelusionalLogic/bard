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
#ifndef XLIB_COLOR_H
#define XLIB_COLOR_H

#include <X11/Xresource.h>
#include <stdbool.h>
#include <stdlib.h>
#include "pipestage.h"
#include "unit.h"

#define MAXCOLOR 16
#define COLORLEN 10
extern const char* colorCode[MAXCOLOR];
extern const char* colorName[MAXCOLOR];

struct XlibColor {
	char colormem[MAXCOLOR * COLORLEN];
	char (*color)[COLORLEN];
	XrmDatabase rdb;
};

struct PipeStage color_getStage();

//Parses a string for color
int color_parseString(struct XlibColor* cobj, char* input, Vector* output);

#endif
