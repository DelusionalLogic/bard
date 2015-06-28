#include "unit.h"
#include <stdlib.h>
#include <string.h>

void unit_init(struct Unit* unit) {
	unit->name = NULL;
	unit->type = UNIT_POLL;
	unit->command = NULL;

	unit->hasRegex = false;
	unit->regex = NULL;

	unit->advancedFormat = false;
	unit->format = NULL;

	unit->interval = 0;
}

void unit_free(struct Unit* unit) {
	free(unit->name);
	free(unit->command);
	free(unit->regex);
	free(unit->format);
}

	//Setters
	bool unit_setName(struct Unit* unit, const char* name) {
		free(unit->name);
		if(name == NULL) {
			unit->name = NULL;
			return true;
		}

		size_t nameLen = strlen(name) + 1;
		unit->name = malloc(sizeof(char) * nameLen);
		if(unit->name == NULL) return false;
		strcpy(unit->name, name);
		return true;
	}
	bool unit_setType(struct Unit* unit, const enum UnitType type) {
		unit->type = type;
		return true;
	}
	bool unit_setCommand(struct Unit* unit, const char* command) {
		free(unit->command);
		if(command == NULL) {
			unit->command = NULL;
			return true;
		}

		size_t commandLen = strlen(command) + 1;
		unit->command = malloc(sizeof(char) * commandLen);
		if(unit->command == NULL) return false;
		strcpy(unit->command, command);
		return true;
	}
	bool unit_setRegex(struct Unit* unit, const char* regex) {
		free(unit->regex);
		if(regex == NULL) {
			unit->regex = NULL;
			return true;
		}

		size_t regexLen = strlen(regex) + 1;
		unit->regex = malloc(sizeof(char) * regexLen);
		if(unit->regex == NULL) return false;
		strcpy(unit->regex, regex);
		return true;
	}
	bool unit_setFormat(struct Unit* unit, const char* format){
		free(unit->format);
		if(format == NULL) {
			unit->format = NULL;
			return true;
		}

		size_t formatLen = strlen(format) + 1;
		unit->format = malloc(sizeof(char) * formatLen);
		if(unit->format == NULL) return false;
		strcpy(unit->format, format);
		return true;
	}
	bool unit_setInterval(struct Unit* unit, const int interval) {
		unit->interval = interval;
		return true;
	}
