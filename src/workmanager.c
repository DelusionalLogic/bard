#include "workmanager.h"
#include <time.h>
#include <unistd.h>
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
};
static bool vecAddUnit(void* elem, void* userdata) {
	struct AddUnitData* data = (struct AddUnitData*)userdata;
	time_t curTime = time(NULL);
	struct UnitContainer container = { .nextRun = curTime, .unit = elem };
	sl_insert(data->list, &container);
	return true;
}

void workmanager_init(struct WorkManager* manager) {
	sl_init(&manager->list, sizeof(struct UnitContainer), unitPlaceComp);
}

void workmanager_free(struct WorkManager* manager) {
	sl_free(&manager->list);
}

void workmanager_addUnits(struct WorkManager* manager, Vector* vec) {
	struct AddUnitData data = { .list = &manager->list };
	vector_foreach(vec, vecAddUnit, &data);
}

void workmanager_run(struct WorkManager* manager, bool (*execute)(struct Unit* unit), bool (*render)()) {
	struct UnitContainer* container = (struct UnitContainer*)sl_get(&manager->list, 0);
	while(true)
	{

		time_t curTime = time(NULL);
		if(container->nextRun >= curTime)
			sleep(container->nextRun - curTime);

		if(!execute(container->unit))
			break;
		
		curTime = time(NULL);
		container->nextRun = curTime + container->unit->interval;
		sl_reorder(&manager->list, 0);
		container = (struct UnitContainer*)sl_get(&manager->list, 0);
		if(container->nextRun != curTime)
			render();
	}
}
