#ifndef UNITEXECUTE_H
#define UNITEXECUTE_H

#include <setjmp.h>
#include "pipestage.h"

struct PipeStage unitexec_getStage();

bool unitexec_process(jmp_buf, void*, struct Unit* unit);

#endif
