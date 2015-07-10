#ifndef UNITEXECUTE_H
#define UNITEXECUTE_H

#include "pipestage.h"

struct PipeStage unitexec_getStage();

int unitexec_process(void*, struct Unit* unit);

#endif
