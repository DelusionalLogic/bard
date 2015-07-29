#include "barconfig.h"
#include <stdbool.h>
#include <errno.h>
#include "fs.h"
#include "vector.h"
#include "configparser.h"
#include "strcolor.h"

int barconfig_init(void* obj, char* configDir);
int barconfig_kill(void* obj);
int barconfig_getArg(void* obj, char* out, size_t outSize);

struct PipeStage barconfig_getStage() {
	struct PipeStage stage;
	stage.enabled = true;
	stage.obj = malloc(sizeof(Vector));
	if(stage.obj == NULL)
		stage.error = ENOMEM;
	stage.create = barconfig_init;
	stage.addUnits = NULL;
	stage.getArgs = barconfig_getArg;
	stage.colorString = NULL;
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
	return true;
}

static bool background(Vector* arg, const char* option) {
	if(option == NULL)
		return true;
	vector_putListBack(arg, " -B \"", 5);
	char* out;
	colorize(option, &out);
	vector_putListBack(arg, out, strlen(out));
	free(out);
	vector_putListBack(arg, "\"", 1);
	return true;
}

static bool foreground(Vector* arg, const char* option) {
	if(option == NULL)
		return true;
	vector_putListBack(arg, " -F \"", 5);
	char* out;
	colorize(option, &out);
	vector_putListBack(arg, out, strlen(out));
	free(out);
	vector_putListBack(arg, "\"", 1);
	return true;
}

int barconfig_init(void* obj, char* configDir) {
	vector_init(obj, sizeof(char), 128);
	vector_putListBack(obj, configDir, strlen(configDir)+1);
	return 0;
}

int barconfig_getArg(void* obj, char* out, size_t outSize) {
	Vector args;
	vector_init(&args, sizeof(char), 128);
	struct ConfigParser parser;
	struct ConfigParserEntry entry[] = {
		StringConfigEntry("bar:geometry", geometry, NULL),
		StringConfigEntry("bar:background", background, NULL),
		StringConfigEntry("bar:foreground", foreground, NULL),
	};
	cp_init(&parser, entry);
	char* path = pathAppend(((Vector*)obj)->data, "bard.conf");
	cp_load(&parser, path, &args);
	free(path);
	cp_kill(&parser);
	vector_putBack(&args, "\0");

	size_t argLen = strlen(out);
	if(argLen + vector_size(&args) > outSize)
		return ENOMEM;
	strcpy(out + argLen, args.data);
	return 0;
}

int barconfig_kill(void* obj) {
	vector_kill(obj); //OBJ IS A VECTOR
	return 0;
}
