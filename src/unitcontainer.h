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
#ifndef UNITCONTAINER_H
#define UNITCONTAINER_H

#include "vector.h"

struct Units {
	Vector left;
	Vector center;
	Vector right;
};

void units_init(jmp_buf jmpBuf, struct Units* units);
void units_free(struct Units* units);

void units_load(jmp_buf jmpBuf, struct Units* units, char* configDir);
int units_preprocess(struct Units* units);

#endif
