#ifndef COLOR_H
#define COLOR_H

#include <stdbool.h>
#include <stdlib.h>
#include "unit.h"

//Load whatever colors you need from the place where they are stored
void color_init(char* configRoot);
void color_free();

//Should take text and replace it with the colored string.
bool color_parseColor(char* text, size_t maxSize);


#endif
