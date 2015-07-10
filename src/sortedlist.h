#ifndef SORTEDLIST_H
#define SORTEDLIST_H

#include <stdbool.h>
#include "linkedlist.h"

typedef bool (*sortCompar)(void* obj, void* other);

struct SortedList {
	LinkedList list;
	sortCompar comparator;
};

void sl_init(struct SortedList* list, size_t elementSize, sortCompar comp);
void sl_kill(struct SortedList* list);

int sl_insert(struct SortedList* list, void* element);

void* sl_get(struct SortedList* list, size_t index);
void sl_reorder(struct SortedList* list, size_t index);
void sl_remove(struct SortedList* list, size_t index);
void* sl_getAndRemove(struct SortedList* list, size_t index);

int sl_size(struct SortedList* list);

#endif
