#include "conf.h"
#include <stdlib.h>
#include <string.h>

void conf_init(struct Conf* conf) {
	conf->separator = NULL;
}

void conf_free(struct Conf* conf) {
	free(conf->separator);
}

bool conf_setSeparator(struct Conf* conf, const char* sep) {
		printf("\n\n!%s!\n\n\n",sep);
		free(conf->separator);
		if(sep == NULL) {
			conf->separator = NULL;
			return true;
		}

		size_t nameLen = strlen(sep) + 1;
		conf->separator = malloc(sizeof(char) * nameLen);
		if(conf->separator == NULL) return false;
		strcpy(conf->separator, sep);
		return true;
}
