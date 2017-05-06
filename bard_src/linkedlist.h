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
#ifndef LINKEDLIST_H
#define LINKEDLIST_H

#include <stdlib.h>
#include <setjmp.h>
#include <stdbool.h>

struct llElement{
	struct llElement* next;
	void* data;
};

typedef struct{
	size_t elementSize;
	size_t length;
	struct llElement *first;
	struct llElement *last;
} LinkedList;

typedef bool (*Callback)(void* element, void* userdata);

void ll_init(LinkedList* list, size_t elementSize);
void ll_kill(LinkedList* list);

void* ll_insert(LinkedList* list, size_t index, void* data);
void* ll_get(LinkedList* list, size_t index);

bool ll_foreach(LinkedList* list, Callback cb, void* userdata);

void ll_remove(LinkedList* list, size_t index);

int ll_size(LinkedList* list);

#endif
