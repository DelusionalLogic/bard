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
#ifndef OUTPUT_H
#define OUTPUT_H

#include "unitcontainer.h"
#include <setjmp.h>
#include "vector.h"
#include "align.h"
#include "unit.h"

struct Output {
	char* separator;
	Vector out[ALIGN_NUM];
	int maxMon;
};

void out_init(jmp_buf jmpBuf, struct Output* output, char* configDir);
void out_kill(struct Output* output);

void out_addUnits(jmp_buf jmpBuf, struct Output* output, struct Units* units);

char* out_format(jmp_buf jmpBuf, struct Output* output, struct Unit* unit);

#endif
