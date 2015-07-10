#ifndef FORMATTER_H
#define FORMATTER_H

#include "unit.h"
#include "linkedlist.h"

struct Formatter {
	LinkedList bufferList;
}; 

void formatter_init(struct Formatter* formatter);
void formatter_kill(struct Formatter* formatter);

bool formatter_format(struct Formatter* formatter, struct Unit* unit, const char* input, char* output, size_t outLen);

#endif
