#ifndef CONF_H
#define CONF_H

#include <stdbool.h>

struct Conf {
	char* separator;
	char* geometry;
};

void conf_init(struct Conf* conf);
void conf_free(struct Conf* conf);

bool conf_setSeparator(struct Conf* conf, const char* name);
bool conf_setGeometry(struct Conf* conf, const char* geometry);

#endif
