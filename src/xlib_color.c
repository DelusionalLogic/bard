//Xlib color provider. Replaces color strings with colors found in the X resource database
#include "color.h"
#include <iniparser.h>
#include <X11/Xresource.h>
#include <ctype.h>
#include "fs.h"
#include "vector.h"

#define MAXCOLOR 16
#define COLORLEN 10
char* colorName[MAXCOLOR] = {
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
char colormem[MAXCOLOR * COLORLEN] = { 0xBA, 0xBE };
char (*color)[COLORLEN] = (char (*)[COLORLEN])colormem;
XrmDatabase rdb;

void color_init(char* configPath) {
	log_write(LEVEL_INFO, "Getting config for display\n");
	Display* display  = XOpenDisplay(NULL);
	XrmInitialize();
	rdb = XrmGetDatabase(display);
	if(!rdb){
		//Stolen from awesomewm, seems like quite the hack
		(void)XGetDefault(display, "", "");
		rdb = XrmGetDatabase(display);
		if(!rdb)
			log_write(LEVEL_ERROR, "Failed opening the X resource manager");
	}

	//Load those colors from the Xrm
	char* resType;
	XrmValue res;
	for(int i = 0; i < MAXCOLOR; i++) {
		int resCode = XrmGetResource(rdb, colorName[i], NULL, &resType, &res);
		if(resCode && (strcmp(resType, "String")) == 0){
			log_write(LEVEL_INFO, "%s\n", res.addr);
			snprintf(color[i], 4, "#AA");
			size_t cnt = 0;
			while(*(res.addr + cnt) != '\0') {
				*(color[i] + 3 + cnt) = toupper(*(res.addr + 1 + cnt));
				cnt++;
			}
		}
	}
	XCloseDisplay(display);
}

void color_free() {
	XrmDestroyDatabase(rdb);
}

#define LOOKUP_MAX 10
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

bool color_parseColor(struct Unit* unit, char* output, size_t maxSize) {
	Vector newOut;
	vector_init(&newOut, sizeof(char), maxSize); 
	
	char lookupmem[MAXCOLOR*LOOKUP_MAX] = {0}; //the string we are looking for. Depending on the MAX_MATCH this might have to be longer
	char (*lookup)[LOOKUP_MAX] = (char (*)[LOOKUP_MAX])lookupmem;
	for(int i = 0; i < MAXCOLOR; i++)
	{
		snprintf(lookup[i], LOOKUP_MAX, "$%s", colorName[i]); //This should probably be computed at compiletime
		//TODO: Error checking
	}	 
	size_t formatLen = strlen(output)+1;
	const char* curPos = output;
	const char* prevPos = NULL;
	while(curPos < output + formatLen)
	{
		prevPos = curPos;
		int index = 0;
		curPos = getNext(curPos, &index, lookup, LOOKUP_MAX);

		if(curPos == NULL)
			break;

		vector_putListBack(&newOut, prevPos, curPos-prevPos);

		vector_putListBack(&newOut, "%{F", 3);
		vector_putListBack(&newOut, color[index], strlen(color[index]));
		vector_putListBack(&newOut, "}", 1);
		curPos += strlen(lookup[index]);
	}
	vector_putListBack(&newOut, prevPos, output + formatLen - prevPos);
	if(vector_size(&newOut) > maxSize) {
		log_write(LEVEL_ERROR, "Output for %s too long", unit->name);
		return false;
	}
	strncpy(output, newOut.data, vector_size(&newOut));
	return true;
}
