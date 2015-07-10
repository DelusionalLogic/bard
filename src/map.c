#include "map.h"
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

bool defaultCompar(const void* e1, const void* e2, size_t eSize) {
	return memcmp(e1, e2, eSize) == 0;
}

int map_init(struct Map* map, size_t keySize, size_t valueSize, bool (*compar)(const void* e1, const void* e2, size_t eSize)) {
	vector_init(&map->keys, keySize, 10);
	vector_init(&map->values, valueSize, 10);
	map->compar = compar != NULL ? compar : defaultCompar;
	return 0;
}

int map_kill(struct Map* map) {
	vector_kill(&map->keys);
	vector_kill(&map->values);
	return 0;
}

int map_put(struct Map* map, const void* key, const void* value) {
	vector_putBack(&map->keys, key);
	vector_putBack(&map->values, value);
	return 0;
}

struct findKeyData {
	const void* key;
	size_t keySize;
	int index;
	bool (*compar)(const void*, const void*, size_t);
};
#define SEARCH_DONE 1
#define SEARCH_CONT 0
static int findKey(void* elem, void* userdata) {
	struct findKeyData* data = (struct findKeyData*)userdata;
	if(data->compar(elem, data->key, data->keySize))
		return SEARCH_DONE;
	data->index++;
	return SEARCH_CONT;
}
static int getIndex(struct Map* map, const void* key) {
	struct findKeyData data = {
		.key = key,
		.keySize = map->keys.elementSize,
		.index = 0,
		.compar = map->compar,
	};
	if(vector_foreach(&map->keys, findKey, &data) != SEARCH_DONE)
		return -1;
	return data.index;
}

void* map_get(struct Map* map, const void* key) {
	int index = getIndex(map, key);
	if(index == -1)
		return NULL;
	return vector_get(&map->values, index);
}

int map_remove(struct Map* map, const void* key) {
	int index = getIndex(map, key);
	if(index == -1)
		return ENOTFOUND;
	vector_remove(&map->keys, index);
	vector_remove(&map->values, index);
	return 0;
}

struct forEachCallData {
	int index;
	int (*callback)(void*, void*, void*);
	void* userdata;
	struct Map* map;
};
static int forEachCall(void* elem, void* userdata) {
	struct forEachCallData* data = (struct forEachCallData*)userdata;
	void* value = vector_get(&data->map->values, data->index++);
	return data->callback(elem, value, data->userdata);
}
int map_foreach(struct Map* map, int (*callback)(void* key, void* value, void* userdata), void* userdata) {
	struct forEachCallData data = {
		.index = 0,
		.callback = callback,
		.userdata = userdata,
		.map = map,
	};
	return vector_foreach(&map->keys, forEachCall, &data); 
}

int map_size(struct Map* map) {
	return vector_size(&map->keys);
}
