#include "formatarray.h"
#include "logger.h"

void formatarray_kill(jmp_buf jmpBuf, struct FormatArray* fmtArray) {
	char* index = malloc(fmtArray->longestKey * sizeof(char)); //TODO: WHAT THE HELL C?
	index[0] = '\0';
	char** val;
	JSLF(val, fmtArray->array, index);
	while(val != NULL) {
		if(*val != NULL)
			free(*val);
		JSLN(val, fmtArray->array, index);
	}
	int bytes;
	JSLFA(bytes, fmtArray->array);
	free(index);
}
