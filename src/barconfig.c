#include "barconfig.h"
#include <stdbool.h>
#include <errno.h>
#include "fs.h"
#include "vector.h"
#include "configparser.h"

int barconfig_init(void* obj, char* configDir);
int barconfig_kill(void* obj);
int barconfig_getArg(void* obj, char* out, size_t outSize);

struct PipeStage barconfig_getStage() {
	struct PipeStage stage;
	stage.obj = malloc(sizeof(Vector));
	if(stage.obj == NULL)
		stage.error = ENOMEM;
	stage.create = barconfig_init;
	stage.addUnits = NULL;
	stage.getArgs = barconfig_getArg;
	stage.process = NULL;
	stage.destroy = barconfig_kill;
	return stage;
}

static bool geometry(Vector* arg, const char* option) {
	if(option == NULL)
		return true;
	vector_putListBack(arg, " -g \"", 5);
	vector_putListBack(arg, option, strlen(option));
	vector_putListBack(arg, "\"", 1);
	log_write(LEVEL_INFO, "HELLO WORLD");
	return true;
}

int barconfig_init(void* obj, char* configDir) {
	vector_init(obj, sizeof(char), 128);
	struct ConfigParser parser;
	struct ConfigParserEntry entry[] = {
		StringConfigEntry("bar:geometry", geometry, NULL),
	};
	cp_init(&parser, entry);
	char* path = pathAppend(configDir, "bard.conf");
	cp_load(&parser, path, obj);
	free(path);
	cp_kill(&parser);
	vector_putBack(obj, "\0");
	return 0;
}

int barconfig_getArg(void* obj, char* out, size_t outSize) {
	size_t argLen = strlen(out);
	if(argLen + vector_size(obj) > outSize)
		return ENOMEM;
	strcpy(out + argLen, ((Vector*)obj)->data);
	return 0;
}

int barconfig_kill(void* obj) {
	vector_kill(obj); //OBJ IS A VECTOR
	return 0;
}
