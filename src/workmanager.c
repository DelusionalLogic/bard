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
#include "workmanager.h"
#include <time.h>
#include <unistd.h>
#include <sys/select.h>
#include <string.h>
#include "logger.h"

struct UnitContainer {
	time_t nextRun;
	struct Unit* unit;
};

static bool unitPlaceComp(void* obj, void* oth) {
	struct UnitContainer* this = (struct UnitContainer*)obj;
	struct UnitContainer* other = (struct UnitContainer*)oth;

	if(this->nextRun <= other->nextRun)
		return true;
	return false;
}

struct AddUnitData {
	struct SortedList* list;
	Vector* pipeList;
	fd_set* fdset;
};
static int vecAddUnit(void* elem, void* userdata) {
	struct AddUnitData* data = (struct AddUnitData*)userdata;
	struct Unit* unit = (struct Unit*)elem;
	time_t curTime = time(NULL);
	if(unit->type == UNIT_POLL) {
		struct UnitContainer container = { .nextRun = curTime, .unit = elem };
		int err = sl_insert(data->list, &container);
		if(err)
			return err;
	}else if(unit->type == UNIT_RUNNING) {
		int err = vector_putBack(data->pipeList, &elem);
		if(err)
			return err;
		if(unit->pipe != -1)
			FD_SET(unit->pipe, data->fdset);
	}
	return 0;
}

void workmanager_init(struct WorkManager* manager) {
	FD_ZERO(&manager->fdset);
	sl_init(&manager->list, sizeof(struct UnitContainer), unitPlaceComp);
	vector_init(&manager->pipeList, sizeof(struct Unit*), 8);
}

void workmanager_kill(struct WorkManager* manager) {
	sl_kill(&manager->list);
	vector_kill(&manager->pipeList);
}

void workmanager_addUnits(struct WorkManager* manager, struct Units *units) {
	struct AddUnitData data = { 
		.list = &manager->list,
		.pipeList = &manager->pipeList,
		.fdset = &manager->fdset,
	};
	vector_foreach(&units->left, vecAddUnit, &data);
	vector_foreach(&units->center, vecAddUnit, &data);
	vector_foreach(&units->right, vecAddUnit, &data);
}

struct pipeRunData {
	fd_set* fdset;
	int (*execute)(struct Unit* unit);
	bool update;
};
int pipeRun(void* elem, void* userdata) {
	struct pipeRunData* data = (struct pipeRunData*)userdata;
	struct Unit* unit = *(struct Unit**)elem;
	log_write(LEVEL_INFO, "A pipe got ready for unit %s", unit->name);
	if(unit->pipe == -1)
		return 0;

	if(FD_ISSET(unit->pipe, data->fdset)) {
		int err = data->execute(unit);
		if(!err)
			data->update = true;
	}
	return 0;
}

static bool isDone(struct WorkManager* manager) {
	time_t curTime = time(NULL);
	struct UnitContainer* container = (struct UnitContainer*)sl_get(&manager->list, 0);
	if(container->nextRun <= curTime)
		return false; //We have a poll unit ready to run

	struct timeval tval = {
		.tv_sec = 0,
		.tv_usec = 0,
	};
	fd_set newSet;
	memcpy(&newSet, &manager->fdset, sizeof(manager->fdset));
	if(select(FD_SETSIZE, &newSet, NULL, NULL, &tval) != 0)
		return false; //We have a running unit ready to read from the pipe (It might finish this tick)

	return true;
}	

int workmanager_run(struct WorkManager* manager, int (*execute)(struct Unit* unit), int (*render)()) {
	struct UnitContainer* container = (struct UnitContainer*)sl_get(&manager->list, 0);
	bool doRender = false;
	while(true)
	{
		time_t curTime = time(NULL);

		//Wait for the next unit or a pipe is ready
		if(container->nextRun > curTime) {
			struct timeval tval = {
				.tv_sec = container->nextRun - curTime,
				.tv_usec = 0,
			};
			fd_set newSet;
			memcpy(&newSet, &manager->fdset, sizeof(fd_set));
			int ready = select(FD_SETSIZE, &newSet, NULL, NULL, &tval);
			if(ready > 0) {
				struct pipeRunData data = {
					.fdset = &newSet,
					.execute = execute,
					.update = false,
				};
				int err = vector_foreach(&manager->pipeList, pipeRun, &data);
				if(err)
					return err;

				doRender = data.update;
				goto render;
			}
		}

		int err = execute(container->unit);
		if(err > 0)
			return err;
		if(err != -1) //We don't need to rerender
			doRender = true;
		
		curTime = time(NULL);
		container->nextRun = curTime + container->unit->interval;
		sl_reorder(&manager->list, 0);
		container = (struct UnitContainer*)sl_get(&manager->list, 0);
render:
		if(isDone(manager) && doRender){
			doRender = false;
			int err = render();
			if(err)
				return err;
		}
	}
}
