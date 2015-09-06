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
#ifndef FORMATTER_H
#define FORMATTER_H

#include "pipestage.h"
#include "unit.h"
#include "linkedlist.h"

struct Formatter {
	LinkedList bufferList;
}; 

struct PipeStage formatter_getStage();

int formatter_init(struct Formatter* formatter, char* configDir);
int formatter_kill(struct Formatter* formatter);

int formatter_format(struct Formatter* formatter, struct Unit* unit);

#endif
