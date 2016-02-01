#ifndef REGEX_H
#define REGEX_H

#include <stdbool.h>
#include <Judy.h>
#include <setjmp.h>
#include "formatarray.h"
#include "unitcontainer.h"
#include "unit.h"

struct Regex{
	Pvoid_t regexCache;
	size_t maxLen;
};

void regex_init(struct Regex* regex);
void regex_kill(struct Regex* regex);

bool regex_compile(jmp_buf jmpBuf, struct Regex* regex, struct Units* units);
bool regex_match(jmp_buf jmpBuf, struct Regex* regex, struct Unit* unit, char* str, struct FormatArray* array);

#endif
