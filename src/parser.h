#ifndef PARSER_H
#define PARSER_H

#include <stdbool.h>
#include "cst.h"
#include "unit.h"

int parser_compileStr(const char* str, Vector* nodes);

#endif
