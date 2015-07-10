#ifndef OUTPUT_H
#define OUTPUT_H

#include "unitcontainer.h"
#include "vector.h"
#include "align.h"
#include "conf.h"

struct Output {
	struct Conf* conf;
	Vector out[ALIGN_NUM];
};

void out_init(struct Output* output, struct Conf* conf);
void out_free(struct Output* output);

void out_insert(struct Output* output, struct Units* units);

char* out_format(struct Output* output);

#endif
