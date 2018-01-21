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
#include <errno.h>
#include <setjmp.h>
#include "myerror.h"
#include "logger.h"
#include "gdbus/dbus.h"

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
};
static bool vecAddUnit(void* elem, void* userdata) {
	struct AddUnitData* data = (struct AddUnitData*)userdata;
	struct Unit* unit = (struct Unit*)elem;
	time_t curTime = time(NULL);

	//TODO: remove the static here and do that somewhere else
	if(unit->type == UNIT_POLL || unit->type == UNIT_STATIC) {
		struct UnitContainer container = { .nextRun = curTime, .unit = elem };
		sl_insert(data->list, &container);
		if(error_waiting()) {
			unit_kill(unit);
			THROW_CONT(false, "While adding %s to the workqueue", unit->name);
		}
	}else if(unit->type == UNIT_RUNNING) {
		vector_putBack(data->pipeList, &elem);
		if(error_waiting()) {
			unit_kill(unit);
			THROW_CONT(false, "While adding %s to the workqueue", unit->name);
		}
	}
	return true;
}

void workmanager_init(struct WorkManager* manager, struct RunnerBuffer* buffers, struct Dbus* dbus) {
	manager->fdset = runner_getfds(buffers);
	FD_SET(dbus_getfd(dbus), &manager->fdset); //Get events from dbus

	manager->buffer = buffers;
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
	};
	vector_foreach(&units->left, vecAddUnit, &data);
	VPROP_THROW("While adding the left side");
	vector_foreach(&units->center, vecAddUnit, &data);
	VPROP_THROW("While adding the cetner");
	vector_foreach(&units->right, vecAddUnit, &data);
	VPROP_THROW("While adding the right side");
}

struct pipeRunData {
	fd_set* fdset;
	struct RunnerBuffer* buffers;
	Vector unitsReady;
};
bool pipeRun(void* elem, void* userdata) {
	struct pipeRunData* data = (struct pipeRunData*)userdata;
	struct Unit* unit = *(struct Unit**)elem;
	if(unit->type != UNIT_RUNNING)
		return true;

	if(runner_ready(data->buffers, data->fdset, unit)) {
		log_write(LEVEL_INFO, "A pipe got ready for unit %s", unit->name);
		vector_putBack(&data->unitsReady, &unit);
	}
	return true;
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
	int status = select(FD_SETSIZE, &newSet, NULL, NULL, &tval);
	if(status == -1) {
		switch(errno) {
			case EBADF:
				THROW_NEW(false, "One of \"running\" unit commands exited");
			case EINTR:
				THROW_NEW(false, "The select was interrupted");
		}
	} else if (status >= 1) {
		return false; //We have a running unit ready to read from the pipe (It might finish this tick)
	}

	return true;
}	

bool workmanger_waiting(struct WorkManager* manager) {
	return !isDone(manager);
}

enum WorkType workmanager_next(struct WorkManager* manager, struct Dbus* dbus, union Work* work) {
	struct UnitContainer* container = (struct UnitContainer*)sl_get(&manager->list, 0);
	if(error_waiting())
		THROW_CONT(WT_NULL, "While getting the next unit to run");
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
		if(ready == -1) {
			switch(errno) {
				case EBADF:
					THROW_NEW(WT_NULL, "One of the \"running\" units exited");
				case EINTR:
					THROW_NEW(WT_NULL, "The select was interrupted");
			}
		} else if(ready >= 1) { //TODO: We only actually support a single event at a time, but during startup multiple get ready at the same time, so we lie
			log_write(LEVEL_INFO, "%d events fired", ready);
			//Check for dbus event
			if(FD_ISSET(dbus_getfd(dbus), &newSet)) {
				log_write(LEVEL_INFO, "Dbus work!");
				struct DbusWork* dwork = NULL;
				read(dbus_getfd(dbus), &dwork, sizeof(struct DbusWork*));
				work->dbus = dwork;
				return WT_DBUS;
			}
			//Check for unit event
			struct pipeRunData data = {
				.fdset = &newSet,
				.buffers = manager->buffer,
			};
			vector_init(&data.unitsReady, sizeof(struct Unit*), ready);
			bool hitEnd = vector_foreach(&manager->pipeList, pipeRun, &data);
			if(error_waiting())
				THROW_CONT(WT_NULL, "While searching for the waiting unit");
			if(vector_size(&data.unitsReady) > 0) {
				work->unit.unitsReady = data.unitsReady;
				return WT_UNIT;
			}
		}
	}
	struct Unit* nextUnit = container->unit;
	curTime = time(NULL);
	container->nextRun = curTime + nextUnit->interval;
	sl_reorder(&manager->list, 0);
	if(error_waiting())
		THROW_CONT(WT_NULL, "While reordering the work queue");
	vector_init(&work->unit.unitsReady, sizeof(struct Unit*), 1);
	vector_putBack(&work->unit.unitsReady, &nextUnit);
	return WT_UNIT;
}
