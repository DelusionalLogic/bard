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
#include "map.h"
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

bool defaultCompar(jmp_buf jmpBuf, const void* e1, const void* e2, size_t eSize) {
	return memcmp(e1, e2, eSize) == 0;
}

void map_init(jmp_buf jmpBuf, struct Map* map, size_t keySize, size_t valueSize, bool (*compar)(jmp_buf, const void* e1, const void* e2, size_t eSize)) {
	vector_init(jmpBuf, &map->keys, keySize, 10);
	vector_init(jmpBuf, &map->values, valueSize, 10);
	map->compar = compar != NULL ? compar : defaultCompar;
}

void map_kill(struct Map* map) {
	vector_kill(&map->keys);
	vector_kill(&map->values);
}

void map_put(jmp_buf jmpBuf, struct Map* map, const void* key, const void* value) {
	vector_putBack(jmpBuf, &map->keys, key);
	vector_putBack(jmpBuf, &map->values, value);
}

struct findKeyData {
	const void* key;
	size_t keySize;
	int index;
	bool (*compar)(jmp_buf jmpBuf, const void*, const void*, size_t);
};
static bool findKey(jmp_buf jmpBuf, void* elem, void* userdata) {
	struct findKeyData* data = (struct findKeyData*)userdata;
	if(data->compar(jmpBuf, elem, data->key, data->keySize))
		return false;
	data->index++;
	return true;
}
static int getIndex(jmp_buf jmpBuf, struct Map* map, const void* key) {
	struct findKeyData data = {
		.key = key,
		.keySize = map->keys.elementSize,
		.index = 0,
		.compar = map->compar,
	};
	if(vector_foreach(jmpBuf, &map->keys, findKey, &data))
		return -1;
	return data.index;
}

void* map_get(jmp_buf jmpBuf, struct Map* map, const void* key) {
	int index = getIndex(jmpBuf, map, key);
	if(index == -1)
		return NULL;
	return vector_get(jmpBuf, &map->values, index);
}

void map_remove(jmp_buf jmpBuf, struct Map* map, const void* key) {
	int index = getIndex(jmpBuf, map, key);
	vector_remove(&map->keys, index);
	vector_remove(&map->values, index);
}

struct forEachCallData {
	int index;
	bool (*callback)(jmp_buf, void*, void*, void*);
	void* userdata;
	struct Map* map;
};
static bool forEachCall(jmp_buf jmpBuf, void* elem, void* userdata) {
	struct forEachCallData* data = (struct forEachCallData*)userdata;
	void* value = vector_get(jmpBuf, &data->map->values, data->index++);
	return data->callback(jmpBuf, elem, value, data->userdata);
}
bool map_foreach(jmp_buf jmpBuf, struct Map* map, bool (*callback)(jmp_buf, void*, void*, void*), void* userdata) {
	struct forEachCallData data = {
		.index = 0,
		.callback = callback,
		.userdata = userdata,
		.map = map,
	};
	return vector_foreach(jmpBuf, &map->keys, forEachCall, &data); 
}

int map_size(struct Map* map) {
	return vector_size(&map->keys);
}
