#ifndef LINKEDLIST_H
#define LINKEDLIST_H

#include <stdlib.h>
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

typedef int (*Callback)(void* element, void* userdata);

void ll_init(LinkedList* list, size_t elementSize);
void ll_delete(LinkedList* list);

void* ll_insert(LinkedList* list, size_t index, void* data);
void* ll_get(LinkedList* list, size_t index);

int ll_foreach(LinkedList* list, Callback cb, void* userdata);

void ll_remove(LinkedList* list, size_t index);

int ll_size(LinkedList* list);

#endif
