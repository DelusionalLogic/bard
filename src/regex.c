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
	pcre2_code** val;
	JSLG(val,  regex->regexCache, unit->name);
	if(val == NULL)
		longjmp(jmpBuf, MYERR_UNITWRONGTYPE);

	strcpy(array->name, "regex");

	//Unit has no regex
	{
		char** val;
		JSLI(val, array->array, "1");
		*val = malloc(strlen(string));
		strcpy(*val, string);
	}

	if(*val == NULL)
		return true;


	pcre2_match_data* match_data = pcre2_match_data_create_from_pattern(*val, NULL);
	int rc = pcre2_match(*val,
			string,
			strlen(string),
			0,
			0,
			match_data,
			NULL);

	if (rc < 0)
	{
		switch(rc)
		{
			case PCRE2_ERROR_NOMATCH: printf("No match\n"); break;
									  /*
										 Handle other special cases if you like
										 */
			default: printf("PCRE2 matching error %d\n", rc); break;
		}
		pcre2_match_data_free(match_data);   /* Release memory used for the match */
		return 1;
	}
	PCRE2_SIZE* ovector = pcre2_get_ovector_pointer(match_data);


	char num[5]; //I don't think anyone will ever match more than 9999 things in bard
	for (int i = 0; i < rc; i++)
	{
		char* substring_start = string + ovector[2*i];
		size_t substring_length = ovector[2*i+1] - ovector[2*i];
		snprintf(num, sizeof(num), "%d", i+1);

		size_t numLen = strlen(num);
		if(numLen > array->longestKey)
			array->longestKey = numLen;

		char** val;
		JSLI(val, array->array, num);
		*val = malloc(substring_length+1);
		strncpy(*val, substring_start, substring_length);
		(*val)[substring_length] = '\0';
		log_write(LEVEL_INFO, "regex group %s is %s", num, *val);
	}
	array->longestKey = 5;

	//TODO: NAMED REGEX MATCHING

	return true;
}
