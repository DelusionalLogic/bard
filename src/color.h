#ifndef COLOR_H
#define COLOR_H

#include <stdbool.h>
#include "unit.h"

//Load whatever colors you need from the place where they are stored
void color_init(char* configRoot);
void color_free();

//Should insert colors in the correct places in the unit, replacing the format string as it goes.
//If the format string is too short the function has the responsibility to allocate a new one and free the previous
//NOT YET: If the "Advanced Format" flag is set, the function should set the correct values in the Format_Env
bool color_parseColor(struct Unit* unit);


#endif
