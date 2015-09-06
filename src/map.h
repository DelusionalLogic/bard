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
#ifndef MAP_H
#define MAP_H

#include "vector.h"
#include <stdbool.h>

#define ENOTFOUND 132

struct Map {
	Vector keys;
	Vector values;
	bool (*compar)(jmp_buf, const void*, const void*, size_t);
};

void map_init(jmp_buf jmpBuf, struct Map* map, size_t keySize, size_t valueSize, bool (*compar)(jmp_buf, const void*, const void*, size_t));
void map_kill(struct Map* map);

void map_put(jmp_buf jmpBuf, struct Map* map, const void* key, const void* value);
void* map_get(jmp_buf jmpBuf, struct Map* map, const void* key);

void map_remove(jmp_buf jmpBuf, struct Map* map, const void* key);

bool map_foreach(jmp_buf jmpBuf, struct Map* map, bool (*callback)(jmp_buf, void*, void*, void*), void* userdata);

int map_size(struct Map* map);

#endif
