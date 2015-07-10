#ifndef FONT_H
#define FONT_H

#include "vector.h"

struct Font {
	Vector fonts;
};

void font_init(struct Font* font);
void font_free(struct Font* font);

int font_insert(struct Font* font, Vector* units);

#endif
