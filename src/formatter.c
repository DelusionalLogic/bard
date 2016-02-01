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

bool formatter_format(jmp_buf jmpBuf, char* input, struct FormatArray arrays[], size_t arraysCnt, char** Poutput) {
	Vector output;
	vector_init(jmpBuf, &output, sizeof(char), 512);
	size_t inLen = strlen(input);
	size_t start = 0;
	for(size_t i = 0; i < inLen; i++) {
		if(input[i] == '\\') {
			if(input[i+1] == '\\') {
				vector_putListBack(jmpBuf, &output, input + start, (i-start));
				vector_putListBack(jmpBuf, &output, "\\", 1);
				i++;
				start = i+1;
			}else if(input[i+1] == '$') {
				vector_putListBack(jmpBuf, &output, input + start, (i-start));
				vector_putListBack(jmpBuf, &output, "$", 1);
				i++;
				start = i+1;
			}else{
				log_write(LEVEL_ERROR, "No escape code \\%c", input[i]);
				return false;
			}
		}else if(input[i] == '$') {
			vector_putListBack(jmpBuf, &output, input + start, i-start);

			struct FormatArray* formatArr = NULL;
			{
				Vector ident;
				vector_init(jmpBuf, &ident, sizeof(char), 12);

				i++;
				while(input[i] != '[') {
					vector_putBack(jmpBuf, &ident, input + i);
					i++;
				}
				vector_putListBack(jmpBuf, &ident, "\0", 1);

				for(size_t i = 0; i < arraysCnt; i++) {
					if(strcmp(arrays[i].name, ident.data) == 0) {
						formatArr = &arrays[i];
						break;
					}
				}

				if(formatArr == NULL) {
					log_write(LEVEL_ERROR, "No array named %s", ident.data);
					vector_kill(&ident);
					return false;
				}

				vector_kill(&ident);
			}

			char* value;
			{
				Vector key;
				vector_init(jmpBuf, &key, sizeof(char), 12);

				i++;
				while(input[i] != ']') {
					vector_putBack(jmpBuf, &key, input + i);
					i++;
				}
				vector_putListBack(jmpBuf, &key, "\0", 1);

				PWord_t pval;
				JSLG(pval, formatArr->array, key.data);

				if(pval == NULL) {
					log_write(LEVEL_ERROR, "No key named %s", key.data);
					return false;
					vector_kill(&key);
				}

				value = *pval;

				vector_kill(&key);
			}

			vector_putListBack(jmpBuf, &output, value, strlen(value));
			start = i+1;
		}
	}
	vector_putListBack(jmpBuf, &output, input + start, (inLen - start)+1);
	*Poutput = vector_detach(&output);
	return true;
}
