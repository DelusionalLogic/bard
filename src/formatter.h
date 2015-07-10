#ifndef FORMATTER_H
#define FORMATTER_H

#include "pipestage.h"
#include "unit.h"
#include "linkedlist.h"

struct Formatter {
	LinkedList bufferList;
}; 

struct PipeStage formatter_getStage();

int formatter_init(struct Formatter* formatter, char* configDir);
int formatter_kill(struct Formatter* formatter);

int formatter_format(struct Formatter* formatter, struct Unit* unit);

#endif
