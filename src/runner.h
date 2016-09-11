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
#ifndef RUNNER_H
#define RUNNER_H

#include <Judy.h>
#include <stdbool.h>
#include <setjmp.h>
#include <unistd.h>
#include <sys/select.h>
#include "unitcontainer.h"
#include "unit.h"

struct RunnerBuffer{
	Pvoid_t cbuff; //Command to buffer
	Pvoid_t ubuff; //Unit to buffer
	size_t longestKey;
	Pvoid_t owners;
};

void runner_startPipes(struct RunnerBuffer* buffers, struct Units* units);
void runner_stopPipes(struct RunnerBuffer* buffers);
fd_set runner_getfds(struct RunnerBuffer* buffers);
bool runner_ready(struct RunnerBuffer* buffers, fd_set* fdset, struct Unit* unit);
void runner_read(struct RunnerBuffer* buffers, struct Unit* unit, char** const out);

#endif
