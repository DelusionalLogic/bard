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
