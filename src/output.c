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

void out_set(struct Outputs* outs, struct Unit* unit, char* in) {
	char** str;

	JLI(str, outs->outputs, (Word_t)unit);
	if(*str != NULL)
		free(*str);
	*str = in;
}

char* out_format(struct Outputs* outs, struct Units* container, int monitors, const char* separator) {
	Vector* aligns[] = {&container->left, &container->center, &container->right};
	Vector vec;
	vector_init(&vec, sizeof(char), 128);
	PROP_THROW(NULL, "While formatting output");
	size_t sepLen = strlen(separator);
	for(int mon = 0; mon < monitors; mon++) {
		char monStr[33];
		int monStrLen = snprintf(monStr, 33, "%d", mon);
		vector_putListBack(&vec, "%{S", 3);
		PROP_THROW(NULL, "While adding monitor output");
		vector_putListBack(&vec, monStr, monStrLen);
		PROP_THROW(NULL, "While adding monitor output");
		vector_putListBack(&vec, "}", 1);
		PROP_THROW(NULL, "While adding monitor output");
		for(int i = ALIGN_FIRST; i <= ALIGN_LAST; i++) {
			vector_putListBack(&vec, AlignStr[i], strlen(AlignStr[i]));
			PROP_THROW(NULL, "While adding alignment output");
			size_t index = 0;
			struct Unit* unit = vector_getFirst(aligns[i], &index);
			PROP_THROW(NULL, "While iterating unit outputs");
			bool first = true;
			while(unit != NULL) {
				char** str;
				JLG(str, outs->outputs, (Word_t)unit);
				if(str != NULL && str != NULL && *str != NULL && unit->render) {
					vector_putListBack(&vec, "%{F-}%{B-}%{T-}", 15);
					PROP_THROW(NULL, "While iterating unit outputs");

					if(!first) {
						vector_putListBack(&vec, separator, sepLen);
						PROP_THROW(NULL, "While iterating unit outputs");
					}
					first = false;

					vector_putListBack(&vec, "%{F-}%{B-}%{T-}", 15);
					PROP_THROW(NULL, "While iterating unit outputs");

					vector_putListBack(&vec, *str, strlen(*str));
					PROP_THROW(NULL, "While iterating unit outputs");
				}
				unit = vector_getNext(aligns[i], &index);
				PROP_THROW(NULL, "While iterating unit outputs");
			}
		}
	}
	//Remember to add the terminator back on
	static char term = '\0';
	vector_putBack(&vec, &term);
	PROP_THROW(NULL, "While adding output terminator");
	//Copy into new buffer owned by calling function
	char* buff = vector_detach(&vec);
	return buff;
}
