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
