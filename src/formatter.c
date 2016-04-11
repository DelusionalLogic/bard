//Copyright (C) 2015 Jesper Jensen
//    This file is part of bard.
//
//    bard is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    bard is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with bard.  If not, see <http://www.gnu.org/licenses/>.
#include "formatter.h"
#include <Judy.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "myerror.h"
#include "logger.h"

struct Parser {
	const char* str;
	char cur;
	size_t index;
	size_t len;
};

void parser_start(struct Parser* parser, const char* str) {
	parser->str = str;
	parser->index = 0;
	parser->len = strlen(str);
	parser->cur = parser->len == 0 ? '\0' : parser->str[parser->index];
}

bool parser_isDone(struct Parser* parser) {
	return parser->index > parser->len;
}

void parser_eat(struct Parser* parser) {
	if(parser_isDone(parser))
		return;
	parser->index++;

	parser->cur = parser_isDone(parser) ? '\0' : parser->str[parser->index];
}

char* parseArrayData(jmp_buf jmpBuf, struct Parser* parser, const struct FormatArray *arrays[], const size_t arraysCnt);

bool formatter_format(jmp_buf jmpBuf, const char* input, const struct FormatArray *arrays[], size_t arraysCnt, char** Poutput) {
	Vector output;
	vector_init(jmpBuf, &output, sizeof(char), 512);

	struct Parser parser;
	struct Parser* p = &parser;
	parser_start(p, input);

	size_t start = 0;
	while(!parser_isDone(p)) {
		if(p->cur == '\\') {
			parser_eat(p);
			if(p->cur == '\\') {
				vector_putListBack(jmpBuf, &output, "\\", 1);
				parser_eat(p);
				start = p->index+1;
			}else if(p->cur == '$') {
				vector_putListBack(jmpBuf, &output, "$", 1);
				parser_eat(p);
				start = p->index+1;
			}else{
				log_write(LEVEL_ERROR, "No escape code \\%c", p->cur);
				return false;
			}
		}else if(p->cur == '$') {
			parser_eat(p);
			char* value = parseArrayData(jmpBuf, p, arrays, arraysCnt);
			vector_putListBack(jmpBuf, &output, value, strlen(value));
		}else {
			vector_putBack(jmpBuf, &output, &p->cur);
		}
		parser_eat(p);
	}
	vector_putListBack(jmpBuf, &output, input + start, (p->len - start)+1);
	*Poutput = vector_detach(&output);
	return true;
}

char* parseArrayData(jmp_buf jmpBuf, struct Parser * p, const struct FormatArray *arrays[], const size_t arraysCnt) {
	//Find the array
	const struct FormatArray* formatArr = NULL;
	{
		Vector ident;
		vector_init(jmpBuf, &ident, sizeof(char), 12);

		while(p->cur != '[' && !parser_isDone(p)) {
			vector_putBack(jmpBuf, &ident, &p->cur);
			parser_eat(p);
		}
		vector_putListBack(jmpBuf, &ident, "\0", 1);

		for(size_t i = 0; i < arraysCnt; i++) {
			if(strcmp(arrays[i]->name, ident.data) == 0) {
				formatArr = arrays[i];
				break;
			}
		}

		if(formatArr == NULL) {
			log_write(LEVEL_ERROR, "No array named %s", ident.data);
			vector_kill(&ident);
			longjmp(jmpBuf, MYERR_USERINPUTERR);
		}

		vector_kill(&ident);
	}

	//Find the value
	char* value;
	{
		Vector key;
		vector_init(jmpBuf, &key, sizeof(char), 12);

		parser_eat(p);
		while(p->cur != ']' && !parser_isDone(p)) {
			vector_putBack(jmpBuf, &key, &p->cur);
			parser_eat(p);
		}
		vector_putListBack(jmpBuf, &key, "\0", 1);

		PWord_t pval;
		JSLG(pval, formatArr->array, (uint8_t*)key.data);

		if(pval == NULL) {
			log_write(LEVEL_INFO, "No key named \"%s\" in \"%s\"", key.data, formatArr->name);
			value = ""; //Set that to empty since theres no value
		} else {
			value = (char*)*pval;
		}


		vector_kill(&key);
	}
	return value;
}
