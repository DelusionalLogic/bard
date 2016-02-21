#include "formatarray.h"
#include "logger.h"

void formatarray_kill(jmp_buf jmpBuf, struct FormatArray* fmtArray) {
	char index[fmtArray->longestKey];
	char** val;
	JSLF(val, fmtArray->array, index);
	while(val != NULL) {
		free(*val);
		JSLN(val, fmtArray->array, index);
	}
	int bytes;
	JSLFA(bytes, fmtArray->array);
	log_write(LEVEL_INFO, "Format array was %d bytes long", bytes);
}
