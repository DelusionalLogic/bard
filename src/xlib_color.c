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

const char* colorCode[MAXCOLOR] = {
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
const char* colorName[MAXCOLOR] = {
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

void color_init(jmp_buf jmpBuf, void* obj, char* configPath);
void color_kill(void* obj);
bool color_parseColor(jmp_buf jmpBuf, void* obj, struct Unit* unit);

struct PipeStage color_getStage(jmp_buf jmpBuf) {
	struct PipeStage stage;
	stage.enabled = true;
	stage.obj = malloc(sizeof(struct XlibColor));
	if(stage.obj == NULL)
		longjmp(jmpBuf, MYERR_ALLOCFAIL);
	stage.create = color_init;
	stage.addUnits = NULL;
	stage.getArgs = NULL;
	stage.colorString = NULL;
	stage.colorString = color_parseString;
	stage.process = color_parseColor;
	stage.destroy = color_kill;
	return stage;
}

void color_init(jmp_buf jmpBuf, void* obj, char* configPath) {	
	struct XlibColor* cobj = (struct XlibColor*)obj;

	cobj->color = (char (*)[COLORLEN])cobj->colormem;

	log_write(LEVEL_INFO, "Getting config for display");
	Display* display  = XOpenDisplay(NULL);
	XrmInitialize();
	cobj->rdb = XrmGetDatabase(display);
	if(!cobj->rdb){
		//Stolen from awesomewm, seems like quite the hack
		(void)XGetDefault(display, "", "");
		cobj->rdb = XrmGetDatabase(display);
		if(!cobj->rdb) {
			log_write(LEVEL_WARNING, "Failed opening the X resource manager");
			longjmp(jmpBuf, MYERR_XOPEN);
		}
	}

	//Load those colors from the Xrm
	char* resType;
	XrmValue res;
	for(int i = 0; i < MAXCOLOR; i++) {
		int resCode = XrmGetResource(cobj->rdb, colorCode[i], NULL, &resType, &res);
		if(resCode && (strcmp(resType, "String")) == 0){
			snprintf(cobj->color[i], 4, "#FF");
			size_t cnt = 0;
			while(*(res.addr + cnt) != '\0') {
				*(cobj->color[i] + 3 + cnt) = toupper(*(res.addr + 1 + cnt));
				cnt++;
			}
		}
	}
	XCloseDisplay(display); //This also destroys the database object (rdb)
}

void color_kill(void* obj) {
}

#define LOOKUP_MAX 16
static char* getNext(const char* curPos, int* index, char (*lookups)[LOOKUP_MAX], size_t lookupsLen)
{
	char* curMin = strstr(curPos, lookups[0]);
	*index = 0;
	char* thisPos = NULL;
	for(size_t i = 1; i < lookupsLen; i++)
	{
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

void color_parseString(jmp_buf jmpBuf, struct XlibColor* cobj, char* input, Vector* output) {
	char lookupmem[(MAXCOLOR * 2)*LOOKUP_MAX] = {0}; //Mutliply by two because each color has two variants 
	char (*lookup)[LOOKUP_MAX] = (char (*)[LOOKUP_MAX])lookupmem;
	for(int i = 0; i < MAXCOLOR; i++)
	{
		snprintf(lookup[i], LOOKUP_MAX, "$color[%d]", i); //This should probably be computed at compiletime
		snprintf(lookup[i+16], LOOKUP_MAX, "$color[%s]", colorName[i]); //This should probably be computed at compiletime
		//TODO: Error checking
	}	 
	size_t formatLen = strlen(input)+1;
	const char* curPos = input;
	const char* prevPos = NULL;
	while(curPos < input + formatLen)
	{
		prevPos = curPos;
		int index = 0;
		curPos = getNext(curPos, &index, lookup, MAXCOLOR*2);

		if(curPos == NULL)
			break;

		int colorNum = index % 16; //We need to find the "shorter" code for the color
		vector_putListBack(jmpBuf, output, prevPos, curPos-prevPos);
		vector_putListBack(jmpBuf, output, cobj->color[colorNum], strlen(cobj->color[colorNum]));
		curPos += strlen(lookup[index]);
	}
	vector_putListBack(jmpBuf, output, prevPos, input + formatLen - prevPos);
}

bool color_parseColor(jmp_buf jmpBuf, void* obj, struct Unit* unit) {
	struct XlibColor* cobj = (struct XlibColor*)obj;
	Vector newOut;
	vector_init(jmpBuf, &newOut, sizeof(char), UNIT_BUFFLEN); 

	jmp_buf parseEx;
	int errCode = setjmp(parseEx);
	if(errCode == 0) {
		color_parseString(jmpBuf, cobj, unit->buffer, &newOut);
	} else {
		log_write(LEVEL_ERROR, "Couldn't parse the color in %s", unit->name);
		longjmp(jmpBuf, errCode);
	}

	if(vector_size(&newOut) > UNIT_BUFFLEN) {
		log_write(LEVEL_ERROR, "No space in unit buffer");
		log_write(LEVEL_ERROR, "-- While parsing color in %s", unit->name);
		longjmp(jmpBuf, MYERR_NOSPACE);
	}
	strncpy(unit->buffer, newOut.data, vector_size(&newOut));
	vector_kill(&newOut);
	return true;
}
