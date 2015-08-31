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

void workmanager_init(jmp_buf jmpBuf, struct WorkManager* manager);
void workmanager_kill(struct WorkManager* manager);

void workmanager_addUnits(jmp_buf jmpBuf, struct WorkManager* manager, struct Units* units);
int workmanager_run(jmp_buf jmpBuf, struct WorkManager* manager, bool (*execute)(jmp_buf jmpBuf, struct Unit* unit), void (*render)(jmp_buf jmpBuf));
#endif
