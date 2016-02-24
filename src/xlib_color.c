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
//Xlib color provider. Replaces color strings with colors found in the X resource database
#include "xlib_color.h"
#include <iniparser.h>
#include <ctype.h>
#include <errno.h>
#include "myerror.h"
#include "fs.h"
#include "vector.h"

static const char* colorCode[MAXCOLOR] = {
	"color0",
   	"color1",
	"color2",
	"color3",
	"color4",
	"color5",
	"color6",
   	"color7",
	"color8",
	"color9",
	"color10",
	"color11",
	"color12",
	"color13",
	"color14",
	"color15",
};
static const char* colorName[MAXCOLOR] = {
	"black",
	"red",
	"green",
	"yellow",
	"blue",
	"magenta",
	"cyan",
	"white",
	"grey",
	"brightred",
	"brightgreen",
	"brightyellow",
	"brightblue",
	"brightmagenta",
	"brightcyan",
	"brightwhite",
};

void xcolor_loadColors(jmp_buf jmpBuf, struct XlibColor* obj) {	
	obj->color = (char (*)[COLORLEN])obj->colormem;

	log_write(LEVEL_INFO, "Getting config for display");
	Display* display  = XOpenDisplay(NULL);
	XrmInitialize();
	obj->rdb = XrmGetDatabase(display);
	if(!obj->rdb){
		//Stolen from awesomewm, seems like quite the hack
		(void)XGetDefault(display, "", "");
		obj->rdb = XrmGetDatabase(display);
		if(!obj->rdb) {
			log_write(LEVEL_WARNING, "Failed opening the X resource manager");
			longjmp(jmpBuf, MYERR_XOPEN);
		}
	}

	//Load those colors from the Xrm
	char* resType;
	XrmValue res;
	for(int i = 0; i < MAXCOLOR; i++) {
		int resCode = XrmGetResource(obj->rdb, colorCode[i], NULL, &resType, &res);
		if(resCode && (strcmp(resType, "String")) == 0){
			snprintf(obj->color[i], 4, "#FF");
			size_t cnt = 0;
			while(*(res.addr + cnt) != '\0') {
				*(obj->color[i] + 3 + cnt) = toupper(*(res.addr + 1 + cnt));
				cnt++;
			}
		}
	}
	XCloseDisplay(display); //This also destroys the database object (rdb)
}

bool xcolor_formatArray(jmp_buf jmpBuf, struct XlibColor* xcolor, struct Unit* unit, struct FormatArray* array) {
	PWord_t val;

	strcpy(array->name, "xcolor");

	for(int i = 0; i < MAXCOLOR; i++) {
		JSLI(val, array->array, colorName[i]);
		*val = xcolor->color[i];
	}

	return true;
}
