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
#include "output.h"
#include <stdio.h>
#include <string.h>
#include "myerror.h"
#include "fs.h"
#include "configparser.h"
#include "vector.h"
#include "logger.h"
#include "unit.h"

void out_kill(struct Outputs* outs) {
	if(outs->outputs == NULL)
		return;
}

void out_set(jmp_buf jmpBuf, struct Outputs* outs, struct Unit* unit, char* in) {
	char** str;

	JLI(str, outs->outputs, unit);
	*str = in;
}

char* out_format(jmp_buf jmpBuf, struct Outputs* outs, struct Units* container, int monitors, const char* separator) {
	Vector* aligns[] = {&container->left, &container->center, &container->right};
	Vector vec;
	vector_init(jmpBuf, &vec, sizeof(char), 128);
	size_t sepLen = strlen(separator);
	for(int mon = 0; mon < monitors; mon++) {
		char monStr[33];
		int monStrLen = snprintf(monStr, 33, "%d", mon);
		vector_putListBack(jmpBuf, &vec, "%{S", 3);
		vector_putListBack(jmpBuf, &vec, monStr, monStrLen);
		vector_putListBack(jmpBuf, &vec, "}", 1);
		for(int i = ALIGN_FIRST; i <= ALIGN_LAST; i++) {
			vector_putListBack(jmpBuf, &vec, AlignStr[i], strlen(AlignStr[i]));
			int index;
			struct Unit* unit = vector_getFirst(jmpBuf, aligns[i], &index);
			bool first = true;
			while(unit != NULL) {
				char** str;
				JLG(str, outs->outputs, unit);
				if(str != NULL && str != NULL &&  *str != NULL) {
					vector_putListBack(jmpBuf, &vec, "%{F-}%{B-}%{T-}", 15);
					if(!first)
						vector_putListBack(jmpBuf, &vec, separator, sepLen);
					first = false;
					vector_putListBack(jmpBuf, &vec, "%{F-}%{B-}%{T-}", 15);

					vector_putListBack(jmpBuf, &vec, *str, strlen(*str));
				}
				unit = vector_getNext(jmpBuf, aligns[i], &index);
			}
		}
	}
	//Remember to add the terminator back on
	static char term = '\0';
	vector_putBack(jmpBuf, &vec, &term);
	//Copy into new buffer owned by calling function
	char* buff = vector_detach(&vec);
	return buff;
}
