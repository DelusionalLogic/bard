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
	log_write(LEVEL_INFO, "A pipe got ready for unit %s\n", unit->name);
	if(unit->pipe == -1)
		return 0;

	if(FD_ISSET(unit->pipe, data->fdset)) {
		int err = data->execute(unit);
		if(!err)
			data->update = true;
	}
	return 0;
}

int workmanager_run(struct WorkManager* manager, int (*execute)(struct Unit* unit), int (*render)()) {
	struct UnitContainer* container = (struct UnitContainer*)sl_get(&manager->list, 0);
	while(true)
	{
		time_t curTime = time(NULL);

		//Wait for the next unit or a pipe is ready
		if(container->nextRun >= curTime) {
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

				if(data.update) {
					int err = render();
					if(err)
						return err;
				}
				continue; //This had nothing to do with the timer, so go to next loop
			}
		}

		int err = execute(container->unit);
		if(err)
			return err;
		
		curTime = time(NULL);
		container->nextRun = curTime + container->unit->interval;
		sl_reorder(&manager->list, 0);
		container = (struct UnitContainer*)sl_get(&manager->list, 0);
		if(container->nextRun != curTime){
			int err = render();
			if(err)
				return err;
		}
	}
}
