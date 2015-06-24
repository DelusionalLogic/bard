#ifndef OUTPUT_H
#define OUTPUT_H

#include "vector.h"
#include "align.h"

struct Output {
	Vector out[ALIGN_NUM];
};

void out_init(struct Output* output);
void out_free(struct Output* output);

void out_insert(struct Output* output, enum Align side, Vector* units);

void out_print(struct Output* output);

#endif
