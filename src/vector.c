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

int vector_size(Vector* vector)
{
	assert(vector->elementSize != 0);
	return vector->size;
}
