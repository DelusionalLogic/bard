#include "vector.h"
#include <errno.h>
#include <string.h>

static int resize(Vector* vector, size_t newElem)
{
	if(newElem + vector->size > vector->maxSize)
	{
		size_t newSize = newElem + vector->size;
		while(vector->maxSize < newSize)
			vector->maxSize *= 2;
		void* newMem = realloc(vector->data, vector->maxSize * vector->elementSize);
		if(newMem == NULL)
			return ENOMEM; //TODO: Error out, we couldn't allocate the new mem
		vector->data = newMem;
	}
	return 0;
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

int vector_putBack(Vector* vector, const void* element)
{
	int err = resize(vector, 1);
	if(err)
		return err;
	memcpy(vector->data + vector->size * vector->elementSize, element, vector->elementSize);
	vector->size += 1;
	return 0;
}

int vector_putListBack(Vector* vector, const void* list, const size_t count)
{
	int err = resize(vector, count);
	if(err)
		return err;
	memcpy(vector->data + vector->size * vector->elementSize, list, count * vector->elementSize);
	vector->size += count;
	return 0;
}

void* vector_get(Vector* vector, const size_t count)
{
	if(count >= vector->size)
		return NULL;
	return vector->data + vector->elementSize * count;
}

void vector_remove(Vector* vector, const size_t count)
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

int vector_foreach(Vector* vector, int (*callback)(void* elem, void* userdata), void* userdata)
{
	for(size_t i = 0; i < vector->size; i++)
	{
		void* elem = vector->data + (vector->elementSize * i);
		int err = callback(elem, userdata);
		if(err)	
			return err;
	}
	return 0;
}

int vector_size(Vector* vector)
{
	return vector->size;
}
