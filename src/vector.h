#ifndef VECTOR_H
#define VECTOR_H

#include <stdlib.h>
#include <stdbool.h>

typedef struct {
	size_t maxSize;
	size_t size;
	size_t elementSize;
	char* data;
} Vector;

void vector_init(Vector* vector, size_t elementsize, size_t initialsize);
void vector_kill(Vector* vector);

int vector_putBack(Vector* vector, const void* element);
int vector_putListBack(Vector* vector, const void* list, size_t count);

void* vector_get(Vector* vector, size_t count);

void vector_remove(Vector* vector, size_t count);
void vector_clear(Vector* vector);
void vector_qsort(Vector* vector, int (*compar)(const void *, const void*));

int vector_foreach(Vector* vector, int (*callback)(void* elem, void* userdata), void* userdata);

int vector_size(Vector* vector);

#endif
