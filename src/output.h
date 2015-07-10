#ifndef OUTPUT_H
#define OUTPUT_H

#include "unitcontainer.h"
#include "vector.h"
#include "align.h"
#include "conf.h"
#include "unit.h"

struct Output {
	struct Conf* conf;
	Vector out[ALIGN_NUM];
};

void out_init(struct Output* output, struct Conf* conf);
void out_kill(struct Output* output);

void out_addUnits(struct Output* output, struct Units* units);

char* out_format(struct Output* output, struct Unit* unit);

#endif
