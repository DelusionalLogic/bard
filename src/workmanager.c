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

struct UnitContainer {
	time_t nextRun;
	struct Unit* unit;
};

static bool unitPlaceComp(jmp_buf jmpBuf, void* obj, void* oth) {
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
static bool vecAddUnit(jmp_buf jmpBuf, void* elem, void* userdata) {
	struct AddUnitData* data = (struct AddUnitData*)userdata;
	struct Unit* unit = (struct Unit*)elem;
	time_t curTime = time(NULL);
	jmp_buf addEx;
	int errCode = setjmp(addEx);
	if(errCode == 0) {
		//TODO: remove the static here and do that somewhere else
		if(unit->type == UNIT_POLL || unit->type == UNIT_STATIC) {
			struct UnitContainer container = { .nextRun = curTime, .unit = elem };
			sl_insert(addEx, data->list, &container);
		}else if(unit->type == UNIT_RUNNING) {
			vector_putBack(addEx, data->pipeList, &elem);
		}
	} else if (errCode == MYERR_ALLOCFAIL) {
		log_write(LEVEL_ERROR, "Allocation error while adding unit %s to work queue", unit->name);
		unit_kill(unit);
		longjmp(jmpBuf, errCode);
	} else longjmp(jmpBuf, errCode);
	return true;
}

void workmanager_init(jmp_buf jmpBuf, struct WorkManager* manager, struct RunnerBuffer* buffers) {
	manager->fdset = runner_getfds(jmpBuf, buffers);
	manager->buffer = buffers;
	sl_init(jmpBuf, &manager->list, sizeof(struct UnitContainer), unitPlaceComp);
	vector_init(jmpBuf, &manager->pipeList, sizeof(struct Unit*), 8);
}

void workmanager_kill(struct WorkManager* manager) {
	sl_kill(&manager->list);
	vector_kill(&manager->pipeList);
}

void workmanager_addUnits(jmp_buf jmpBuf, struct WorkManager* manager, struct Units *units) {
	struct AddUnitData data = { 
		.list = &manager->list,
		.pipeList = &manager->pipeList,
	};
	vector_foreach(jmpBuf, &units->left, vecAddUnit, &data);
	vector_foreach(jmpBuf, &units->center, vecAddUnit, &data);
	vector_foreach(jmpBuf, &units->right, vecAddUnit, &data);
}

struct pipeRunData {
	fd_set* fdset;
	struct RunnerBuffer* buffers;
	struct Unit* unitReady;
};
bool pipeRun(jmp_buf jmpBuf, void* elem, void* userdata) {
	struct pipeRunData* data = (struct pipeRunData*)userdata;
	struct Unit* unit = *(struct Unit**)elem;
	if(unit->type != UNIT_RUNNING)
		return true;

	if(runner_ready(jmpBuf, data->buffers, data->fdset, unit)) {
		log_write(LEVEL_INFO, "A pipe got ready for unit %s", unit->name);
		data->unitReady = unit;
		return false;
	}
	return true;
}

static bool isDone(jmp_buf jmpBuf, struct WorkManager* manager) {
	time_t curTime = time(NULL);
	struct UnitContainer* container = (struct UnitContainer*)sl_get(jmpBuf, &manager->list, 0);
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
				log_write(LEVEL_INFO, "One of \"running\" unit commands exited");
				longjmp(jmpBuf, MYERR_BADFD);
				break;
			case EINTR:
				log_write(LEVEL_INFO, "The select was interrupted");
				return isDone(jmpBuf, manager); //I'll just call myself for now and hope that i won't be interrupted
		}
	} else if (status >= 1) {
		return false; //We have a running unit ready to read from the pipe (It might finish this tick)
	}

	return true;
}	

bool workmanger_waiting(jmp_buf jmpBuf, struct WorkManager* manager) {
	return !isDone(jmpBuf, manager);
}

struct Unit* workmanager_next(jmp_buf jmpBuf, struct WorkManager* manager) {
	struct UnitContainer* container = (struct UnitContainer*)sl_get(jmpBuf, &manager->list, 0);
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
					log_write(LEVEL_INFO, "One of \"running\" unit commands exited");
					longjmp(jmpBuf, MYERR_BADFD);
					break;
				case EINTR:
					log_write(LEVEL_INFO, "The select was interrupted");
					break;
			}
		} else if(ready > 0) {
			struct pipeRunData data = {
				.fdset = &newSet,
				.buffers = manager->buffer,
			};
			if(!vector_foreach(jmpBuf, &manager->pipeList, pipeRun, &data))
				return data.unitReady;
		}
	}
	struct Unit* nextUnit = container->unit;
	curTime = time(NULL);
	container->nextRun = curTime + nextUnit->interval;
	sl_reorder(jmpBuf, &manager->list, 0);
	container = (struct UnitContainer*)sl_get(jmpBuf, &manager->list, 0);
	return nextUnit;
}
