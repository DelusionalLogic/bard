#ifndef FORMATARRAY_H
#define FORMATARRAY_H

#include <Judy.h>
#include <setjmp.h>

struct FormatArray{
	char name[64];
	size_t longestKey;
	Pvoid_t array;
};

void formatarray_kill(struct FormatArray* fmtArray);

#endif
