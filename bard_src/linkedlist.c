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

void ll_init(LinkedList* list, size_t elementSize)
{
	list->elementSize = elementSize;
	list->length = 0;
	list->first = list->last;
}

void ll_kill(LinkedList* list)
{
	list->elementSize = 0x72727272;
	while(list->length > 0) {
		ll_remove(list, 0);
		if(error_waiting()) {
			ERROR_CONT("While removing elements before deleting list");
			break;
		}
	}
}

static struct llElement* construct(size_t elementSize, void* data)
{
	void* newDat = malloc(elementSize);
	memcpy(newDat, data, elementSize);
	struct llElement* elem = (struct llElement*)calloc(1, sizeof(struct llElement));
	if(elem == NULL) {
		free(newDat);
		free(elem);
		THROW_NEW(NULL, "Failed allocating linked list element");
	}
	elem->data = newDat;
	return elem;
}

void* ll_insert(LinkedList* list, size_t index, void* data)
{
	if(index > list->length)
		THROW_NEW(NULL, "Tried to insert value out of range");
	struct llElement* elem = construct(list->elementSize, data);
	PROP_THROW(NULL, "While adding element to linked list");

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

void* ll_get(LinkedList* list, size_t index)
{
	if(index >= list->length)
		THROW_NEW(NULL, "Tried to read beyond list length: %d >= %d", index, list->length);
	struct llElement* cur = list->first;
	for(size_t i = 0; i < index; i++)
		cur = cur->next;
	return cur->data;
}

bool ll_foreach(LinkedList* list, Callback cb, void* userdata)
{
	struct llElement* cur = list->first;
	for(size_t i = 0; i < list->length; i++)
	{
		if(!cb(cur->data, userdata))
			return false;
		cur = cur->next;
	}
	return true;
}

void ll_remove(LinkedList* list, size_t index)
{
	if(index >= list->length)
		VTHROW_NEW("Tried to remove beyond list length: %d >= %d", index, list->length);
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
