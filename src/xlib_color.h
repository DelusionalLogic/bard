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
