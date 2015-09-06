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
char* vector_detach(Vector* vector);

int vector_putBack(Vector* vector, const void* element);
int vector_putListBack(Vector* vector, const void* list, size_t count);

void* vector_get(Vector* vector, size_t count);

void vector_remove(Vector* vector, size_t count);
void vector_clear(Vector* vector);
void vector_qsort(Vector* vector, int (*compar)(const void *, const void*));

int vector_foreach(Vector* vector, int (*callback)(void* elem, void* userdata), void* userdata);

int vector_size(Vector* vector);

#endif
