#include "output.h"
#include <stdio.h>
#include <string.h>
#include "myerror.h"
#include "fs.h"
#include "configparser.h"
#include "vector.h"
#include "logger.h"
#include "unit.h"
#include "strcolor.h"

static void separator(jmp_buf jmpBuf, struct Output* output, const char* separator) {
	if(separator == NULL) {
		output->separator = "";
		return;
	}
	output->separator = malloc(strlen(separator) * sizeof(char));
	if(output->separator == NULL)
		longjmp(jmpBuf, MYERR_ALLOCFAIL);
	char* colors;
	colorize(jmpBuf, separator, &colors);
	strcpy(output->separator, colors);
	free(colors);
}

void out_init(jmp_buf jmpBuf, struct Output* output, char* configDir) {
	struct ConfigParser parser;
	struct ConfigParserEntry entry[] = {
		StringConfigEntry("display:separator", separator, NULL),
	};
	cp_init(jmpBuf, &parser, entry);
	char* path = pathAppend(configDir, "bard.conf");
	cp_load(jmpBuf, &parser, path, output);
	free(path);
	cp_kill(&parser);
	
	for(int i = ALIGN_FIRST; i <= ALIGN_LAST; i++) {
		vector_init(jmpBuf, &output->out[i], sizeof(char*), 10);
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
static bool vecAddUnit(jmp_buf jmpBuf, void* elem, void* userdata) {
	struct AddUnitData* data = (struct AddUnitData*)userdata;
	struct Unit* unit = (struct Unit*)elem;
	char** outPtr = (char**)&unit->buffer;
	vector_putBack(jmpBuf, data->list, &outPtr);
	return true;
}

void out_addUnits(jmp_buf jmpBuf, struct Output* output, struct Units* units) {
	struct AddUnitData data = {
		.list = &output->out[ALIGN_LEFT]
	};
	vector_foreach(jmpBuf, &units->left, vecAddUnit, &data);
	data.list = &output->out[ALIGN_CENTER];
	vector_foreach(jmpBuf, &units->center, vecAddUnit, &data);
	data.list = &output->out[ALIGN_RIGHT];
	vector_foreach(jmpBuf, &units->right, vecAddUnit, &data);
}

struct PrintUnitData {
	bool first;
	char* sep;
	size_t sepLen;
	Vector* vec;
};
static bool vecPrintUnit(jmp_buf jmpBuf, void* elem, void* userdata) {
	struct PrintUnitData* data = (struct PrintUnitData*)userdata;
	char** unit = (char**)elem;
	vector_putListBack(jmpBuf, data->vec, "%{F-}%{B-}%{T-}", 15);
	if(!data->first)
		vector_putListBack(jmpBuf, data->vec, data->sep, data->sepLen);
	vector_putListBack(jmpBuf, data->vec, "%{F-}%{B-}%{T-}", 15);
	vector_putListBack(jmpBuf, data->vec, *unit, strlen(*unit));
	data->first = false;
	return true;
}

//REMEMBER TO FREE THE STRING
char* out_format(jmp_buf jmpBuf, struct Output* output, struct Unit* unit) {
	Vector vec;
	vector_init(jmpBuf, &vec, sizeof(char), 128);
	for(int i = ALIGN_FIRST; i <= ALIGN_LAST; i++) {
		vector_putListBack(jmpBuf, &vec, AlignStr[i], strlen(AlignStr[i]));
		struct PrintUnitData data = {
			.first = true,
			.sep = output->separator,
			.sepLen = strlen(output->separator),
			.vec = &vec,
		};
		vector_foreach(jmpBuf, &output->out[i], vecPrintUnit, &data);
	}
	//Remember to add the terminator back on
	static char term = '\0';
	vector_putBack(jmpBuf, &vec, &term);
	//Copy into new buffer owned by calling function
	char* buff = vector_detach(&vec);
	return buff;
}

