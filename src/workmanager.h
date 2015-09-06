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
#ifndef WORKMANAGER_H
#define WORKMANAGER_H

#include <stdbool.h>
#include <sys/select.h>
#include "unitcontainer.h"
#include "sortedlist.h"
#include "vector.h"
#include "unit.h"

struct WorkManager {
	struct SortedList list;
	Vector pipeList;
	fd_set fdset;
};

void workmanager_init(struct WorkManager* manager);
void workmanager_kill(struct WorkManager* manager);

void workmanager_addUnits(struct WorkManager* manager, struct Units* units);
int workmanager_run(struct WorkManager* manager, int (*execute)(struct Unit* unit), int (*render)());
#endif
