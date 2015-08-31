#ifndef PIPESTAGE_H
#define PIPESTAGE_H

#include "unitcontainer.h"
#include <stdbool.h>
#include <setjmp.h>
#include "unit.h"

struct PipeStage {
	bool enabled;
	void* obj;
	void (*create)(jmp_buf jmpBuf, void* obj, char* configPath);
	void (*addUnits)(jmp_buf jmpBuf, void* obj, struct Units* units); //Temp name
	void (*getArgs)(jmp_buf jmpBuf, void* obj, char* arg, size_t maxLen);
	void (*colorString)(jmp_buf jmpBuf, void* obj, char* str, Vector* vec);
	bool (*process)(jmp_buf jmpBuf, void* obj, struct Unit* unit);
	void (*destroy)(void* obj);
};

#endif
