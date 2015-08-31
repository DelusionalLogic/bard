#ifndef ADVANCEDFORMAT_H
#define ADVANCEDFORMAT_H

#include "pipestage.h"
#include "unit.h"
#include "linkedlist.h"

struct AdvFormatter {
	LinkedList bufferList;
}; 

struct PipeStage advFormatter_getStage();

void advFormatter_init(jmp_buf jmpBuf, struct AdvFormatter* formatter, char* configDir);
void advFormatter_kill(struct AdvFormatter* formatter);

bool advFormatter_format(jmp_buf jmpBuf, struct AdvFormatter* formatter, struct Unit* unit);

#endif
