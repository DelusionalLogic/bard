#ifndef PARSER_H
#define PARSER_H

#include <stdbool.h>
#include "cst.h"
#include "unit.h"

void parser_compileStr(const char* str, Vector* nodes);
void parser_freeCompiled(Vector* compiled);

#endif
