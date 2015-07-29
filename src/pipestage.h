#ifndef PIPESTAGE_H
#define PIPESTAGE_H

#include "unitcontainer.h"
#include <stdbool.h>
#include "unit.h"

struct PipeStage {
	bool enabled;
	int error;
	void* obj;
	int (*create)(void* obj, char* configPath);
	int (*addUnits)(void* obj, struct Units* units); //Temp name
	int (*getArgs)(void* obj, char* arg, size_t maxLen);
	int (*colorString)(void* obj, char* str, Vector* vec);
	int (*process)(void* obj, struct Unit* unit);
	int (*destroy)(void* obj);
};

#endif
