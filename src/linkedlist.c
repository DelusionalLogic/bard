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
#include "linkedlist.h"
#include <string.h>
#include "logger.h"
#include "myerror.h"

void ll_init(jmp_buf jmpBuf, LinkedList* list, size_t elementSize)
{
	list->elementSize = elementSize;
	list->length = 0;
	list->first = list->last;
}

void ll_kill(LinkedList* list)
{
	list->elementSize = 0x72727272;
	jmp_buf jmpBuf;
	int errCode = setjmp(jmpBuf);
	if(errCode == 0) {
		while(list->length > 0)
			ll_remove(jmpBuf, list, 0);
	} else {
		log_write(LEVEL_INFO, "err in ll_kill");
	}//Discard errors. That's too bad
}

static struct llElement* construct(jmp_buf jmpBuf, size_t elementSize, void* data)
{
	void* newDat = malloc(elementSize);
	memcpy(newDat, data, elementSize);
	struct llElement* elem = (struct llElement*)calloc(1, sizeof(struct llElement));
	if(elem == NULL)
		longjmp(jmpBuf, MYERR_ALLOCFAIL);
	elem->data = newDat;
	return elem;
}

void* ll_insert(jmp_buf jmpBuf, LinkedList* list, size_t index, void* data)
{
	if(index > list->length)
		longjmp(jmpBuf, MYERR_OUTOFRANGE);
	struct llElement* elem = construct(jmpBuf, list->elementSize, data);
	if(elem == NULL)
		longjmp(jmpBuf, MYERR_ALLOCFAIL);

	struct llElement* prev = NULL;
	struct llElement* cur = list->first;
	for(size_t i = 0; i < index; i++)
	{
		prev = cur;
		cur = cur->next;
	}
	if(prev == NULL)
		list->first = elem;
	else
		prev->next = elem;
	elem->next = cur;
	list->length++;

	return elem->data;
}

void* ll_get(jmp_buf jmpBuf, LinkedList* list, size_t index)
{
	if(index >= list->length)
		longjmp(jmpBuf, MYERR_OUTOFRANGE);
	struct llElement* cur = list->first;
	for(size_t i = 0; i < index; i++)
		cur = cur->next;
	return cur->data;
}

bool ll_foreach(jmp_buf jmpBuf, LinkedList* list, Callback cb, void* userdata)
{
	struct llElement* cur = list->first;
	for(size_t i = 0; i < list->length; i++)
	{
		if(!cb(jmpBuf, cur->data, userdata))
			return false;
		cur = cur->next;
	}
	return true;
}

void ll_remove(jmp_buf jmpBuf, LinkedList* list, size_t index)
{
	if(index >= list->length)
		longjmp(jmpBuf, MYERR_OUTOFRANGE);
	struct llElement* prev = NULL;
	struct llElement* cur = list->first;
	for(size_t i = 0; i < index; i++)
	{
		prev = cur;
		cur = cur->next;
	}
	if(prev == NULL)
		list->first = cur->next;
	else
		prev->next = cur->next;
	free(cur->data);
	free(cur);
	list->length--;
}

int ll_size(LinkedList* list)
{
	return list->length;
}
