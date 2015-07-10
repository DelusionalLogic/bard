#ifndef MAP_H
#define MAP_H

#include "vector.h"
#include <stdbool.h>

#define ENOTFOUND 132

struct Map {
	Vector keys;
	Vector values;
	bool (*compar)(const void*, const void*, size_t);
};

int map_init(struct Map* map, size_t keySize, size_t valueSize, bool (*compar)(const void*, const void*, size_t));
int map_kill(struct Map* map);

int map_put(struct Map* map, const void* key, const void* value);
void* map_get(struct Map* map, const void* key);

int map_remove(struct Map* map, const void* key);

int map_foreach(struct Map* map, int (*callback)(void* key, void* value, void* userdata), void* userdata);

int map_size(struct Map* map);

#endif
