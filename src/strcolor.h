#ifndef STRCOLOR_H
#define STRCOLOR_H

#include <setjmp.h>

void colorize(jmp_buf jmpBuf, const char* str, char** out);

#endif
