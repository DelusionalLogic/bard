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
