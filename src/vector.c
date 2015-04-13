#include <string.h>
#include "vector.h"

static bool resize(Vector* vector, size_t newElem)
{
	if(newElem + vector->size > vector->maxSize)
	{
		size_t newSize = newElem + vector->size;
		while(vector->maxSize < newSize)
			vector->maxSize *= 2;
		void* newMem = realloc(vector->data, vector->maxSize * vector->elementSize);
		if(newMem == NULL)
			return false; //TODO: Error out, we couldn't allocate the new mem
		vector->data = newMem;
	}
	return true;
}

void vector_init(Vector* vector, size_t elementsize, size_t initialsize)
{
	vector->maxSize = initialsize;
	vector->elementSize = elementsize;
	vector->size = 0;
	vector->data = malloc(initialsize * elementsize);
}

void vector_delete(Vector* vector)
{
	free(vector->data);
}

bool vector_putBack(Vector* vector, void* element)
{
	if(!resize(vector, 1))
		return false;
	memcpy(vector->data + vector->size * vector->elementSize, element, vector->elementSize);
	vector->size += 1;
	return true;
}

bool vector_putListBack(Vector* vector, const void* list, size_t count)
{
	if(!resize(vector, count))
		return false;
	memcpy(vector->data + vector->size * vector->elementSize, list, count * vector->elementSize);
	vector->size += count;
	return true;
}

void* vector_get(Vector* vector, size_t count)
{
	if(count >= vector->size)
		return NULL;
	return vector->data + vector->elementSize * count;
}

void vector_remove(Vector* vector, size_t count)
{
	memmove(vector->data + count * vector->elementSize, vector->data + (count+1) * vector->elementSize, (vector->size-1) * vector->elementSize);
	vector->size -= 1;
}

void vector_clear(Vector* vector)
{
	vector->size = 0;
}

void vector_qsort(Vector* vector, int (*compar)(const void *, const void*))
{
	qsort(vector->data, vector->size, vector->elementSize, compar);
}

void vector_foreach(Vector* vector, bool (*callback)(void* elem, void* userdata), void* userdata)
{
	for(size_t i = 0; i < vector->size; i++)
	{
		void* elem = vector->data + (vector->elementSize * i);
		if(!callback(elem, userdata))
			break;
	}
}

size_t vector_size(Vector* vector)
{
	return vector->size;
}
