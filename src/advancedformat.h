#ifndef ADVANCEDFORMAT_H
#define ADVANCEDFORMAT_H

#include "pipestage.h"
#include "unit.h"
#include "linkedlist.h"

struct AdvFormatter {
	LinkedList bufferList;
}; 

struct PipeStage advFormatter_getStage();

int advFormatter_init(struct AdvFormatter* formatter, char* configDir);
int advFormatter_kill(struct AdvFormatter* formatter);

int advFormatter_format(struct AdvFormatter* formatter, struct Unit* unit);

#endif
