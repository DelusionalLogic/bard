#include "output.h"
#include <stdio.h>
#include <string.h>
#include "fs.h"
#include "configparser.h"
#include "vector.h"
#include "logger.h"
#include "unit.h"

static bool separator(struct Output* output, const char* separator) {
	if(separator == NULL) {
		output->separator = "";
		return true;
	}
	output->separator = malloc(strlen(separator) * sizeof(char));
	if(output->separator == NULL)
		return false;
	strcpy(output->separator, separator);
	return true;
}

void out_init(struct Output* output, char* configDir) {
	struct ConfigParser parser;
	struct ConfigParserEntry entry[] = {
		StringConfigEntry("display:separator", separator, NULL),
	};
	cp_init(&parser, entry);
	char* path = pathAppend(configDir, "bard.conf");
	cp_load(&parser, path, output);
	free(path);
	cp_kill(&parser);
	
	for(int i = ALIGN_FIRST; i <= ALIGN_LAST; i++) {
		vector_init(&output->out[i], sizeof(char*), 10);
	}
}

void out_kill(struct Output* output) {
	for(int i = ALIGN_FIRST; i <= ALIGN_LAST; i++) {
		vector_kill(&output->out[i]);
	}
}

struct AddUnitData {
	Vector* list;
};
static int vecAddUnit(void* elem, void* userdata) {
	struct AddUnitData* data = (struct AddUnitData*)userdata;
	struct Unit* unit = (struct Unit*)elem;
	char** outPtr = (char**)&unit->buffer;
	vector_putBack(data->list, &outPtr);
	return 0;
}

void out_addUnits(struct Output* output, struct Units* units) {
	struct AddUnitData data = {
		.list = &output->out[ALIGN_LEFT]
	};
	vector_foreach(&units->left, vecAddUnit, &data);
	data.list = &output->out[ALIGN_CENTER];
	vector_foreach(&units->center, vecAddUnit, &data);
	data.list = &output->out[ALIGN_RIGHT];
	vector_foreach(&units->right, vecAddUnit, &data);
}

struct PrintUnitData {
	bool first;
	char* sep;
	size_t sepLen;
	Vector* vec;
};
static int vecPrintUnit(void* elem, void* userdata) {
	struct PrintUnitData* data = (struct PrintUnitData*)userdata;
	char** unit = (char**)elem;
	vector_putListBack(data->vec, "%{F-}%{B-}%{T-}", 15);
	if(!data->first)
		vector_putListBack(data->vec, data->sep, data->sepLen);
	vector_putListBack(data->vec, "%{F-}%{B-}%{T-}", 15);
	vector_putListBack(data->vec, *unit, strlen(*unit));
	data->first = false;
	return 0;
}

//REMEMBER TO FREE THE STRING
char* out_format(struct Output* output, struct Unit* unit) {
	Vector vec;
	vector_init(&vec, sizeof(char), 128);
	for(int i = ALIGN_FIRST; i <= ALIGN_LAST; i++) {
		vector_putListBack(&vec, AlignStr[i], strlen(AlignStr[i]));
		struct PrintUnitData data = {
			.first = true,
			.sep = output->separator,
			.sepLen = strlen(output->separator),
			.vec = &vec,
		};
		vector_foreach(&output->out[i], vecPrintUnit, &data);
	}
	//Remember to add the terminator back on
	static char term = '\0';
	vector_putBack(&vec, &term);
	//Copy into new buffer owned by calling function
	char* buff = malloc((vector_size(&vec) * sizeof(char)));
	memcpy(buff, vec.data, vector_size(&vec) * sizeof(char));
	vector_kill(&vec);
	return buff;
}

