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
#ifndef SORTEDLIST_H
#define SORTEDLIST_H

#include <stdbool.h>
#include <setjmp.h>
#include "linkedlist.h"

typedef bool (*sortCompar)(jmp_buf jmpBuf, void* obj, void* other);

struct SortedList {
	LinkedList list;
	sortCompar comparator;
};

void sl_init(jmp_buf jmpBuf, struct SortedList* list, size_t elementSize, sortCompar comp);
void sl_kill(struct SortedList* list);

void sl_insert(jmp_buf jmpBuf, struct SortedList* list, void* element);

void* sl_get(jmp_buf jmpBuf, struct SortedList* list, size_t index);
void sl_reorder(jmp_buf jmpBuf, struct SortedList* list, size_t index);
void sl_remove(jmp_buf jmpBuf, struct SortedList* list, size_t index);
void* sl_getAndRemove(jmp_buf jmpBuf, struct SortedList* list, size_t index);

int sl_foreach(jmp_buf jmpBuf, struct SortedList* list, int (*cb)(void*, void*), void* userdata);

int sl_size(struct SortedList* list);

#endif
