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
#include "vector.h"
#include <setjmp.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include "myerror.h"

static void resize_new(Vector* vector, size_t newElem)
{
	if(newElem + vector->size > vector->maxSize)
	{
		size_t newSize = newElem + vector->size;
		while(vector->maxSize < newSize)
			vector->maxSize *= 2;
		void* newMem = realloc(vector->data, vector->maxSize * vector->elementSize);
		if(newMem == NULL)
			VTHROW_NEW("Failed re-allocating vector array");
		vector->data = newMem;
	}
}
static void resize_assert(Vector* vector, size_t newElem)
{
	if(newElem + vector->size > vector->maxSize)
	{
		size_t newSize = newElem + vector->size;
		while(vector->maxSize < newSize)
			vector->maxSize *= 2;
		void* newMem = realloc(vector->data, vector->maxSize * vector->elementSize);
		assert(newMem != NULL);
		vector->data = newMem;
	}
}

//DEPRECATED
static void resize(jmp_buf jmpBuf, Vector* vector, size_t newElem)
{
	if(newElem + vector->size > vector->maxSize)
	{
		size_t newSize = newElem + vector->size;
		while(vector->maxSize < newSize)
			vector->maxSize *= 2;
		void* newMem = realloc(vector->data, vector->maxSize * vector->elementSize);
		if(newMem == NULL)
			longjmp(jmpBuf, MYERR_ALLOCFAIL);
		vector->data = newMem;
	}
}

void vector_init_assert(Vector* vector, size_t elementsize, size_t initialsize)
{
	vector->maxSize = initialsize;
	vector->elementSize = elementsize;
	vector->size = 0;
	vector->data = malloc(initialsize * elementsize); //This should really not be done here
	assert(vector->data != NULL);
}
void vector_init_new(Vector* vector, size_t elementsize, size_t initialsize)
{
	vector->maxSize = initialsize;
	vector->elementSize = elementsize;
	vector->size = 0;
	vector->data = malloc(initialsize * elementsize); //This should really not be done here
	if(vector->data == NULL)
		VTHROW_NEW("Failed allocating vector array");
}

//DEPRECATED
void vector_init(jmp_buf jmpBuf, Vector* vector, size_t elementsize, size_t initialsize)
{
	vector->maxSize = initialsize;
	vector->elementSize = elementsize;
	vector->size = 0;
	vector->data = malloc(initialsize * elementsize); //This should really not be done here
	if(vector->data == NULL)
		longjmp(jmpBuf, MYERR_ALLOCFAIL);
}

void vector_kill(Vector* vector)
{
	assert(vector->elementSize != 0);
	free(vector->data);
}

char* vector_detach(Vector* vector)
{
	vector->maxSize = 0;
	vector->elementSize = 0;
	vector->size = 0;
	char* oldDat = vector->data;
	vector->data = NULL;
	return oldDat;
}

void vector_putBack_new(Vector* vector, const void* element)
{
	assert(vector->elementSize != 0);

	resize_new(vector, 1);
	if(error_waiting())
		VTHROW_CONT("While appeding to vector");

	memcpy(vector->data + vector->size * vector->elementSize, element, vector->elementSize);
	vector->size += 1;
}
void vector_putBack_assert(Vector* vector, const void* element)
{
	assert(vector->elementSize != 0);

	resize_assert(vector, 1);

	memcpy(vector->data + vector->size * vector->elementSize, element, vector->elementSize);
	vector->size += 1;
}

//DEPRECATED
void vector_putBack(jmp_buf jmpBuf, Vector* vector, const void* element)
{
	assert(vector->elementSize != 0);

	jmp_buf newEx;
	int errCode = setjmp(newEx);
	if(errCode == 0)
		resize(newEx, vector, 1);
	else if(errCode == MYERR_ALLOCFAIL)
		longjmp(jmpBuf, errCode);

	memcpy(vector->data + vector->size * vector->elementSize, element, vector->elementSize);
	vector->size += 1;
}

void vector_putListBack_new(Vector* vector, const void* list, const size_t count)
{
	assert(vector->elementSize != 0);

	resize_new(vector, count);
	if(error_waiting())
		VTHROW_CONT("While appeding list of length %d to vector", count);

	memcpy(vector->data + vector->size * vector->elementSize, list, count * vector->elementSize);
	vector->size += count;
}
int vector_putListBack(jmp_buf jmpBuf, Vector* vector, const void* list, const size_t count)
{
	assert(vector->elementSize != 0);

	jmp_buf newEx;
	int errCode = setjmp(newEx);
	if(errCode == 0)
		resize(jmpBuf, vector, count);
	else if(errCode == MYERR_ALLOCFAIL)
		longjmp(jmpBuf, errCode);

	memcpy(vector->data + vector->size * vector->elementSize, list, count * vector->elementSize);
	vector->size += count;
	return 0;
}

void* vector_get_new(Vector* vector, const size_t count)
{
	assert(vector->elementSize != 0);
	if(count >= vector->size)
		THROW_NEW(NULL, "Tried to read beyond the end of the vector (index was %d, size was %d)", count, vector->size);
	return vector->data + vector->elementSize * count;
}
void* vector_get_assert(Vector* vector, const size_t count)
{
	assert(vector->elementSize != 0);
	assert(count < vector->size);
	return vector->data + vector->elementSize * count;
}
void* vector_get(jmp_buf jmpBuf, Vector* vector, const size_t count)
{
	assert(vector->elementSize != 0);
	if(count >= vector->size)
		longjmp(jmpBuf, MYERR_OUTOFRANGE);
	return vector->data + vector->elementSize * count;
}

void vector_remove(Vector* vector, const size_t count)
{
	assert(vector->elementSize != 0);
	memmove(vector->data + count * vector->elementSize, vector->data + (count+1) * vector->elementSize, (vector->size-1) * vector->elementSize);
	vector->size -= 1;
}

void vector_clear(Vector* vector)
{
	assert(vector->elementSize != 0);
	vector->size = 0;
}

void vector_qsort(Vector* vector, int (*compar)(const void *, const void*))
{
	assert(vector->elementSize != 0);
	qsort(vector->data, vector->size, vector->elementSize, compar);
}

bool vector_foreach_new(Vector* vector, bool (*callback)(void* elem, void* userdata), void* userdata)
{
	assert(vector->elementSize != 0);
	int index = 0;
	void* elem  = vector_getFirst_new(vector, &index); //Should never error out
	while(elem != NULL) {
		bool cont = callback(elem, userdata);
		if(error_waiting())
			THROW_CONT(false, "While inside foreach");
		if(!cont)
			return false;
		elem = vector_getNext_new(vector, &index); //Should never error out
	}
	return true;
}
bool vector_foreach(jmp_buf jmpBuf, Vector* vector, bool (*callback)(jmp_buf jmpBuf, void* elem, void* userdata), void* userdata)
{
	assert(vector->elementSize != 0);
	for(size_t i = 0; i < vector->size; i++)
	{
		void* elem = vector->data + (vector->elementSize * i);
		jmp_buf newEx;
		int errCode = setjmp(newEx);
		if(errCode == 0) {
			if(!callback(newEx, elem, userdata))
				return false;
		}
		else
			longjmp(jmpBuf, errCode); // Any error will make the for loop stop
	}
	return true;
}

void* vector_getFirst_new(Vector* vector, int* index) {
	*index = 0;
	if(*index >= vector_size(vector))
		return NULL;
	return vector_get_new(vector, *index);
}
void* vector_getFirst_assert(Vector* vector, int* index) {
	*index = 0;
	if(*index >= vector_size(vector))
		return NULL;
	return vector_get_assert(vector, *index);
}
void* vector_getFirst(jmp_buf jmpBuf, Vector* vector, int* index) {
	*index = 0;
	if(*index >= vector_size(vector))
		return NULL;
	return vector_get(jmpBuf, vector, *index);
}

void* vector_getNext_new(Vector* vector, int* index) {
	++(*index);
	if(*index >= vector_size(vector))
		return NULL;
	return vector_get_new(vector, *index);
}
void* vector_getNext_assert(Vector* vector, int* index) {
	++(*index);
	if(*index >= vector_size(vector))
		return NULL;
	return vector_get_assert(vector, *index);
}
void* vector_getNext(jmp_buf jmpBuf, Vector* vector, int* index) {
	++(*index);
	if(*index >= vector_size(vector))
		return NULL;
	return vector_get(jmpBuf, vector, *index);
}

int vector_size(Vector* vector)
{
	assert(vector->elementSize != 0);
	return vector->size;
}
