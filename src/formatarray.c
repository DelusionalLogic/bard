#include "formatarray.h"
#include "logger.h"

void formatarray_kill(jmp_buf jmpBuf, struct FormatArray* fmtArray) {
	log_write(LEVEL_INFO, "Freeing formatarray %s", fmtArray->name);
	char* index = malloc(fmtArray->longestKey * sizeof(char)); //TODO: WHAT THE HELL C?
	index[0] = '\0';
	char** val;
	log_write(LEVEL_INFO, "Freeing formatarray %s", fmtArray->name);
	JSLF(val, fmtArray->array, index);
	while(val != NULL) {
		if(*val != NULL)
			free(*val);
		JSLN(val, fmtArray->array, index);
	}
	int bytes;
	JSLFA(bytes, fmtArray->array);
	log_write(LEVEL_INFO, "Format array was %d bytes long", bytes);
	free(index);
}
