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

typedef bool (*Callback)(jmp_buf jmpBuf, void* element, void* userdata);

void ll_init(jmp_buf jmpBuf, LinkedList* list, size_t elementSize);
void ll_kill(LinkedList* list);

void* ll_insert(jmp_buf jmpBuf, LinkedList* list, size_t index, void* data);
void* ll_get(jmp_buf jmpBuf, LinkedList* list, size_t index);

bool ll_foreach(jmp_buf jmpBuf, LinkedList* list, Callback cb, void* userdata);

void ll_remove(jmp_buf jmpBuf, LinkedList* list, size_t index);

int ll_size(LinkedList* list);

#endif
