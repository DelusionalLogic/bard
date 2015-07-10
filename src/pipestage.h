#ifndef PIPESTAGE_H
#define PIPESTAGE_H

#include "unitcontainer.h"
#include "unit.h"

struct PipeStage {
	int error;
	void* obj;
	int (*create)(void* obj, char* configPath);
	int (*addUnits)(void* obj, struct Units* units); //Temp name
	int (*getArgs)(void* obj, char* arg, size_t maxLen);
	int (*process)(void* obj, struct Unit* unit);
	int (*destroy)(void* obj);
};

#endif
