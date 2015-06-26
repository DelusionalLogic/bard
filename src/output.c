#include "output.h"
#include <stdio.h>
#include <string.h>
#include "vector.h"
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
	size_t sepLen;
	Vector* vec;
};
static bool vecPrintUnit(void* elem, void* userdata) {
	struct PrintUnitData* data = (struct PrintUnitData*)userdata;
	char** unit = (char**)elem;
	if(!data->first)
		vector_putListBack(data->vec, data->sep, data->sepLen);
	vector_putListBack(data->vec, *unit, strlen(*unit));
	data->first = false;
	return true;
}

//REMEMBER TO FREE THE STRING
char* out_format(struct Output* output) {
	Vector vec;
	vector_init(&vec, sizeof(char), 128);
	for(int i = ALIGN_FIRST; i <= ALIGN_LAST; i++) {
		vector_putListBack(&vec, AlignStr[i], strlen(AlignStr[i]));
		struct PrintUnitData data = {
			.first = true,
			.sep = output->conf->separator,
			.sepLen = strlen(output->conf->separator),
			.vec = &vec,
		};
		vector_foreach(&output->out[i], vecPrintUnit, &data);
	}
	//Remember to add the terminator back on
	static char term = '\0';
	vector_putBack(&vec, &term);
	//Copy into new buffer owned by calling function
	char* buff = malloc((vector_size(&vec) * sizeof(char)) + 1);
	memcpy(buff, vec.data, vector_size(&vec) * sizeof(char) + 1);
	vector_delete(&vec);
	return buff;
}

