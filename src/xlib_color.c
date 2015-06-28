//Xlib color provider. Replaces color strings with colors found in the X resource database
#include "color.h"
#include <iniparser.h>
#include <X11/Xresource.h>
#include "fs.h"

XrmDatabase rdb;

void color_init(char* configPath) {
	char* displayStr = getenv("DISPLAY");
	log_write(LEVEL_INFO, "Getting config for display: %s", displayStr);
	Display* display  = XOpenDisplay(displayStr);
	rdb = XrmGetDatabase(display);
	XCloseDisplay(display);
}

void color_free() {
	XrmDestroyDatabase(rdb);
}

bool color_parseColor(struct Unit* unit) {
	free(unit->format);
	unit->format = "FooBar"; //TODO: Actually parse the format
							 //And insert colors where they belong
	return true;
}
