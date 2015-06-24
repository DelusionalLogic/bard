#include "sortedlist.h"

void sl_init(struct SortedList* list, size_t elementSize, sortCompar comp) {
	ll_init(&list->list, elementSize);
	list->comparator = comp;
}

void sl_free(struct SortedList* list) {
	ll_delete(&list->list);
}

struct FindPlaceData {
	size_t slot;
	void* elem;
	sortCompar comp;
};
static bool findPlace(void* elem, void* userdata) {
	struct FindPlaceData* data = (struct FindPlaceData*)userdata;
	if(data->comp(data->elem, elem))
		return false;
	data->slot++;
	return true;
}

void sl_insert(struct SortedList* list, void* element) {
	struct FindPlaceData data = { .slot = 0, .elem = element, .comp = list->comparator};
	ll_foreach(&list->list, findPlace, &data);
	ll_insert(&list->list, data.slot, element);
}

void* sl_get(struct SortedList* list, size_t index) {
	return ll_get(&list->list, index);
}

void sl_reorder(struct SortedList* list, size_t index) {
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
	ll_foreach(&list->list, findPlace, &data);
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

void sl_remove(struct SortedList* list, size_t index) {
	return ll_remove(&list->list, index);
}

int sl_size(struct SortedList* list) {
	return ll_size(&list->list);
}
