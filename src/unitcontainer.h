#ifndef UNITCONTAINER_H
#define UNITCONTAINER_H

#include "vector.h"

struct Units {
	Vector left;
	Vector center;
	Vector right;
};

void units_init(struct Units* units);
void units_free(struct Units* units);

int units_load(struct Units* units, char* configDir);

#endif
