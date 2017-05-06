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

void formatter_format(Vector* compiledStr, const struct FormatArray *const arrays[], size_t arraysCnt, char** Poutput) {
	Vector output;
	vector_init(&output, sizeof(char), 512);
		VPROP_THROW("While formatting");

	size_t index = 0;
	struct Node* node = vector_getFirst(compiledStr, &index);
	while(node != NULL) {
		if(node->type == NT_STRING) {
			vector_putListBack(&output, node->str.lit, strlen(node->str.lit));
				VPROP_THROW("While inserting %s", node->str.lit);
		} else if(node->type == NT_ARRAY) {
			for(size_t i = 0; i < arraysCnt; i++) {
				if(strcmp(arrays[i]->name, node->arr.arr) == 0) {
					PWord_t pval;
					JSLG(pval, arrays[i]->array, node->arr.key);
					if(pval != NULL) {
						vector_putListBack(&output, (char*)*pval, strlen((char*)*pval));
							VPROP_THROW("While inserting [%s] -> [%s]", node->arr.key, arrays[i]->name);
					}
					break;
				}
			}
		}
		node = vector_getNext(compiledStr, &index);
	}
	vector_putListBack(&output, "\0", 1);
		VPROP_THROW("While appending null to format string");

	*Poutput = vector_detach(&output);
}
