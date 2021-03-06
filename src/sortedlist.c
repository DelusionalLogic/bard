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
#include "sortedlist.h"
#include <errno.h>
#include <setjmp.h>
#include "myerror.h"

void sl_init(jmp_buf jmpBuf, struct SortedList* list, size_t elementSize, sortCompar comp) {
	ll_init(jmpBuf, &list->list, elementSize);
	list->comparator = comp;
}

void sl_kill(struct SortedList* list) {
	ll_kill(&list->list);
}

struct FindPlaceData {
	size_t slot;
	void* elem;
	sortCompar comp;
};
static bool findPlace(jmp_buf jmpBuf, void* elem, void* userdata) {
	struct FindPlaceData* data = (struct FindPlaceData*)userdata;
	if(data->comp(jmpBuf, data->elem, elem))
		return false;
	data->slot++;
	return true;
}

void sl_insert(jmp_buf jmpBuf, struct SortedList* list, void* element) {
	struct FindPlaceData data = { .slot = 0, .elem = element, .comp = list->comparator};
	ll_foreach(jmpBuf, &list->list, findPlace, &data);
	ll_insert(jmpBuf, &list->list, data.slot, element);
}

void* sl_get(jmp_buf jmpBuf, struct SortedList* list, size_t index) {
	return ll_get(jmpBuf, &list->list, index);
}

void sl_reorder(jmp_buf jmpBuf, struct SortedList* list, size_t index) {
	//Remove the element
	struct llElement* prev = NULL;
	struct llElement* cur = list->list.first;
	for(size_t i = 0; i < index; i++)
	{
		prev = cur;
		cur = cur->next;
	}
	if(prev == NULL)
		list->list.first = cur->next;
	else
		prev->next = cur->next;
	list->list.length--; //Needed to make the foreach behave
	struct llElement* this = cur;
	//Get the new position
	struct FindPlaceData data = { .slot = 0, .elem = cur->data, .comp = list->comparator};
	ll_foreach(jmpBuf, &list->list, findPlace, &data);
	//Reinsert at new position
	cur = list->list.first;
	for(size_t i = 0; i < data.slot; i++)
	{
		prev = cur;
		cur = cur->next;
	}
	if(prev == NULL)
		list->list.first = this;
	else
		prev->next = this;
	list->list.length++;
	this->next = cur;
}

int sl_foreach(jmp_buf jmpBuf, struct SortedList* list, bool (*cb)(jmp_buf, void*, void*), void* userdata) {
	return ll_foreach(jmpBuf, &list->list, cb, userdata);
}

void sl_remove(jmp_buf jmpBuf, struct SortedList* list, size_t index) {
	return ll_remove(jmpBuf, &list->list, index);
}

int sl_size(struct SortedList* list) {
	return ll_size(&list->list);
}
