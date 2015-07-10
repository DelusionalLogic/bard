#include "conf.h"
#include <stdlib.h>
#include <string.h>

void conf_init(struct Conf* conf) {
	conf->separator = NULL;
	conf->geometry = NULL;
}

void conf_kill(struct Conf* conf) {
	free(conf->separator);
	free(conf->geometry);
}

	bool conf_setSeparator(struct Conf* conf, const char* sep) {
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
	bool conf_setGeometry(struct Conf* conf, const char* geometry) {
		free(conf->geometry);
		if(geometry == NULL) {
			conf->separator = NULL;
			return true;
		}

		size_t nameLen = strlen(geometry) + 1;
		conf->geometry = malloc(sizeof(char) * nameLen);
		if(conf->geometry == NULL) return false;
		strcpy(conf->geometry, geometry);
		return true;
	}
