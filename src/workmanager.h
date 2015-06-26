#ifndef WORKMANAGER_H
#define WORKMANAGER_H

#include <stdbool.h>
#include "sortedlist.h"
#include "vector.h"
#include "unit.h"

struct WorkManager {
	struct SortedList list;
};

void workmanager_init(struct WorkManager* manager);
void workmanager_free(struct WorkManager* manager);

void workmanager_addUnits(struct WorkManager* manager, Vector* vec);
void workmanager_run(struct WorkManager* manager, bool (*execute)(struct Unit* unit), bool (*render)());
#endif
