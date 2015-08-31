#ifndef OUTPUT_H
#define OUTPUT_H

#include "unitcontainer.h"
#include <setjmp.h>
#include "vector.h"
#include "align.h"
#include "unit.h"

struct Output {
	char* separator;
	Vector out[ALIGN_NUM];
};

void out_init(jmp_buf jmpBuf, struct Output* output, char* configDir);
void out_kill(struct Output* output);

void out_addUnits(jmp_buf jmpBuf, struct Output* output, struct Units* units);

char* out_format(jmp_buf jmpBuf, struct Output* output, struct Unit* unit);

#endif
