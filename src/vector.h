#ifndef VECTOR_H
#define VECTOR_H

#include <stdlib.h>
#include <stdbool.h>
#include <setjmp.h>

typedef struct {
	size_t maxSize;
	size_t size;
	size_t elementSize;
	char* data;
} Vector;

void vector_init(jmp_buf jmpBuf, Vector* vector, size_t elementsize, size_t initialsize);
void vector_kill(Vector* vector);
char* vector_detach(Vector* vector);

void vector_putBack(jmp_buf jmpBuf, Vector* vector, const void* element);
int vector_putListBack(jmp_buf jmpBuf, Vector* vector, const void* list, const size_t count);

void* vector_get(jmp_buf jmpBuf, Vector* vector, const size_t count);

void vector_remove(Vector* vector, size_t count);
void vector_clear(Vector* vector);
void vector_qsort(Vector* vector, int (*compar)(const void *, const void*));

bool vector_foreach(jmp_buf jmpBuf, Vector* vector, bool (*callback)(jmp_buf jmpBuf, void* elem, void* userdata), void* userdata);

int vector_size(Vector* vector);

#endif
