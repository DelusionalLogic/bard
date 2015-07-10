#ifndef COLOR_H
#define COLOR_H

#include <stdbool.h>
#include <stdlib.h>
#include "pipestage.h"
#include "unit.h"

struct PipeStage color_getStage();

//Load whatever colors you need from the place where they are stored
int color_init(void* obj, char* configRoot);
int color_kill(void* obj);

//Should take text and replace it with the colored string.
int color_parseColor(void* color, struct Unit* unit);


#endif
