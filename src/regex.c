#define PCRE2_CODE_UNIT_WIDTH 8

#include "regex.h"
#include <pcre2.h>
#include <string.h>
#include "logger.h"
#include "myerror.h"

void regex_init(struct Regex* regex) {
	regex->regexCache = NULL;
	regex->maxLen = 0;
}

void regex_kill(struct Regex* regex) {
	PWord_t val;
	char index[regex->maxLen];
	index[0] = '\0';
	JSLF(val, regex->regexCache, index);
	while(val != NULL) {
		pcre2_code_free(*val);
		JSLN(val, regex->regexCache, index);
	}
	int bytes;
	JSLFA(bytes, regex->regexCache);
	log_write(LEVEL_INFO, "Regex cache was %d bytes long", bytes);
}

struct compileRegexData {
	Pvoid_t* arr;
	size_t* maxLen;
};
bool compileRegex(jmp_buf jmpBuf, void* elem, void* userdata) {
	struct Unit* unit = (struct Unit*)elem;
	struct compileRegexData* data = (struct compileRegexData*)userdata;

	int errno;
	PCRE2_SIZE erroroffset;

	pcre2_code* re = pcre2_compile((PCRE2_SPTR)unit->regex,
			PCRE2_ZERO_TERMINATED,
			0,
			&errno,
			&erroroffset,
			NULL);
	PWord_t val;
	JSLI(val, *data->arr, unit->name);
	size_t unitNameLen = strlen(unit->name) + 1;
	if(unitNameLen > *(data->maxLen))
		*(data->maxLen) = unitNameLen;

	if(val == PJERR) {
		longjmp(jmpBuf, MYERR_ALLOCFAIL);
	}
	*val = (unsigned long)re;

	return true;
}

bool regex_compile(jmp_buf jmpBuf, struct Regex* regex, struct Units* units) {
	struct compileRegexData data = {
		&regex->regexCache,
		&regex->maxLen,
	};
	jmp_buf runEx;
	int errCode = setjmp(runEx);
	if(errCode == 0) {
		vector_foreach(runEx, &units->left, compileRegex, &data);
		vector_foreach(runEx, &units->center, compileRegex, &data);
		vector_foreach(runEx, &units->right, compileRegex, &data);
	} else {
		longjmp(jmpBuf, errCode);
	}
	return true;
}

bool regex_match(jmp_buf jmpBuf, struct Regex* regex, struct Unit* unit, char* string, struct FormatArray* array) {
	PWord_t val;
	JSLG(val,  regex->regexCache, unit->name);
	if(val == NULL)
		longjmp(jmpBuf, MYERR_UNITWRONGTYPE);

	strcpy(array->name, "regex");

	//TODO: INSERT A BUNCH OF MATCHING
	JSLI(val, array->array, "a");
	*val = (unsigned long)"TEST";

	return true;
}
