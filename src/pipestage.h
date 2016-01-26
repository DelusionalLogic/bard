//Copyright (C) 2015 Jesper Jensen
//    This file is part of bard.
//
//    bard is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    bard is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with bard.  If not, see <http://www.gnu.org/licenses/>.
#ifndef PIPESTAGE_H
#define PIPESTAGE_H

#include "unitcontainer.h"
#include <stdbool.h>
#include <setjmp.h>
#include "unit.h"

struct PipeStage {
	bool enabled;
	void* obj;
	void (*create)(jmp_buf jmpBuf, void* obj);
	void (*reload)(jmp_buf jmpBuf, void* obj, char* configPath);
	void (*addUnits)(jmp_buf jmpBuf, void* obj, struct Units* units); //Temp name
	void (*getArgs)(jmp_buf jmpBuf, void* obj, char* arg, size_t maxLen);
	void (*colorString)(jmp_buf jmpBuf, void* obj, char* str, Vector* vec);
	bool (*process)(jmp_buf jmpBuf, void* obj, struct Unit* unit);
	void (*destroy)(void* obj);
};

#endif
