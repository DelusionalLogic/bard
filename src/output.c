#include "output.h"
#include <stdio.h>
#include "logger.h"
#include "unit.h"

void out_init(struct Output* output) {
	for(int i = ALIGN_FIRST; i <= ALIGN_LAST; i++) {
		vector_init(&output->out[i], sizeof(char*), 10);
	}
}

void out_free(struct Output* output) {
	for(int i = ALIGN_FIRST; i <= ALIGN_LAST; i++) {
		vector_delete(&output->out[i]);
	}
}

struct AddUnitData {
	Vector* list;
};
static bool vecAddUnit(void* elem, void* userdata) {
	struct AddUnitData* data = (struct AddUnitData*)userdata;
	struct Unit* unit = (struct Unit*)elem;
	char** outPtr = (char**)&unit->output;
	vector_putBack(data->list, &outPtr);
	return true;
}

void out_insert(struct Output* output, enum Align side, Vector* units) {
	struct AddUnitData data = {
		.list = &output->out[side]
	};
	vector_foreach(units, vecAddUnit, &data);
}

static bool vecPrintUnit(void* elem, void* userdata) {
	char** unit = (char**)elem;
	printf("%s", *unit);
	return true;
}

void out_print(struct Output* output) {
	for(int i = ALIGN_FIRST; i <= ALIGN_LAST; i++) {
		printf("%s", AlignStr[i]);
		vector_foreach(&output->out[i], vecPrintUnit, NULL);
	}
	printf("\n");
}
