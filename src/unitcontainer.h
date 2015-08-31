#ifndef UNITCONTAINER_H
#define UNITCONTAINER_H

#include "vector.h"

struct Units {
	Vector left;
	Vector center;
	Vector right;
};

void units_init(jmp_buf jmpBuf, struct Units* units);
void units_free(struct Units* units);

void units_load(jmp_buf jmpBuf, struct Units* units, char* configDir);

#endif
