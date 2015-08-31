#ifndef FORMATTER_H
#define FORMATTER_H

#include "pipestage.h"
#include "unit.h"
#include "linkedlist.h"

struct Formatter {
	LinkedList bufferList;
}; 

struct PipeStage formatter_getStage();

void formatter_init(jmp_buf jmpBuf, struct Formatter* formatter, char* configDir);
void formatter_kill(struct Formatter* formatter);

bool formatter_format(jmp_buf jmpBuf, struct Formatter* formatter, struct Unit* unit);

#endif
