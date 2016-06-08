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
#include "parser.h"

bool formatter_format(jmp_buf jmpBuf, Vector* compiledStr, const struct FormatArray *arrays[], size_t arraysCnt, char** Poutput) {
	Vector output;
	vector_init(jmpBuf, &output, sizeof(char), 512);

	int index = 0;
	struct Node* node = vector_getFirst_new(compiledStr, &index);
	while(node != NULL) {
		if(node->type == NT_STRING) {
			vector_putListBack(jmpBuf, &output, node->str.lit, strlen(node->str.lit));
		} else if(node->type == NT_ARRAY) {
			for(size_t i = 0; i < arraysCnt; i++) {
				if(strcmp(arrays[i]->name, node->arr.arr) == 0) {

					PWord_t pval;
					JSLG(pval, arrays[i]->array, node->arr.key);
					if(pval == NULL) {
						log_write(LEVEL_INFO, "No key named \"%s\" in \"%s\"", node->arr.key, arrays[i]->name);
					} else {
						vector_putListBack(jmpBuf, &output, (char*)*pval, strlen((char*)*pval));
					}
					break;
				}
			}
		}
		node = vector_getNext_new(compiledStr, &index);
	}
	vector_putListBack(jmpBuf, &output, "\0", 1);

	*Poutput = vector_detach(&output);
	return true;
}
