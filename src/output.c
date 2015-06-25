#include "output.h"
#include <stdio.h>
#include "logger.h"
#include "unit.h"

void out_init(struct Output* output, struct Conf* conf) {
	output->conf = conf;
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

struct PrintUnitData {
	bool first;
	char* sep;
};
static bool vecPrintUnit(void* elem, void* userdata) {
	struct PrintUnitData* data = (struct PrintUnitData*)userdata;
	char** unit = (char**)elem;
	if(!data->first)
		printf("%s", data->sep);
	printf("%s", *unit);
	data->first = false;
	return true;
}

void out_print(struct Output* output) {
	for(int i = ALIGN_FIRST; i <= ALIGN_LAST; i++) {
		printf("%s", AlignStr[i]);
		struct PrintUnitData data = { .first = true, .sep = output->conf->separator };
		vector_foreach(&output->out[i], vecPrintUnit, &data);
	}
	printf("\n");
}
