//Copyright (C) 2015 Jesper Jensen
//    This file is part of bard.
//
//    bard is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    bard is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with bard.  If not, see <http://www.gnu.org/licenses/>.
#include "barconfig.h"
#include <stdbool.h>
#include <errno.h>
#include "myerror.h"
#include "fs.h"
#include "vector.h"
#include "configparser.h"
#include "strcolor.h"

void barconfig_init(jmp_buf jmpBuf, void* obj, char* configDir);
void barconfig_kill(void* obj);
void barconfig_getArg(jmp_buf jmpBuf, void* obj, char* out, size_t outSize);

struct PipeStage barconfig_getStage(jmp_buf jmpBuf) {
	struct PipeStage stage;
	stage.enabled = true;
	stage.obj = malloc(sizeof(Vector));
	if(stage.obj == NULL)
		longjmp(jmpBuf, MYERR_ALLOCFAIL);
	stage.create = barconfig_init;
	stage.addUnits = NULL;
	stage.getArgs = barconfig_getArg;
	stage.colorString = NULL;
	stage.process = NULL;
	stage.destroy = barconfig_kill;
	return stage;
}

static void geometry(jmp_buf jmpBuf, Vector* arg, const char* option) {
	if(option == NULL)
		return;
	jmp_buf vecEx;
	int errCode = setjmp(vecEx);
	if(errCode == 0) {
		vector_putListBack(vecEx, arg, " -g \"", 5);
		vector_putListBack(vecEx, arg, option, strlen(option));
		vector_putListBack(vecEx, arg, "\"", 1);
	} else if(errCode == MYERR_ALLOCFAIL) {
		log_write(LEVEL_ERROR, "Failed to allocate more space for geometry string");
		longjmp(jmpBuf, errCode);
	}
}

static void background(jmp_buf jmpBuf, Vector* arg, const char* option) {
	if(option == NULL)
		return;
	jmp_buf vecEx;
	int errCode = setjmp(vecEx);
	if(errCode == 0) {
		vector_putListBack(vecEx, arg, " -B \"", 5);

		jmp_buf colorEx;
		int errCode2 = setjmp(colorEx);
		char* out;
		if(errCode2 == 0) {
			colorize(colorEx, option, &out);
			vector_putListBack(vecEx, arg, out, strlen(out));
			free(out);
			vector_putListBack(vecEx, arg, "\"", 1);
		} else if (errCode2) { //Error trying to allocate out. Lets just put the bare string on there
			vector_putListBack(vecEx, arg, option, strlen(option));
			vector_putListBack(vecEx, arg, "\"", 1);
		}

	} else if(errCode == MYERR_ALLOCFAIL) {
		log_write(LEVEL_ERROR, "Failed to allocate more space for bg string");
		longjmp(jmpBuf, errCode);
	}
}

static void foreground(jmp_buf jmpBuf, Vector* arg, const char* option) {
	if(option == NULL)
		return;
	jmp_buf vecEx;
	int errCode = setjmp(vecEx);
	if(errCode == 0) {
		vector_putListBack(vecEx, arg, " -F \"", 5);
		jmp_buf colorEx;
		int errCode2 = setjmp(colorEx);
		char* out;
		if(errCode2 == 0) {
			colorize(colorEx, option, &out);
			vector_putListBack(vecEx, arg, out, strlen(out));
			free(out);
			vector_putListBack(vecEx, arg, "\"", 1);
		} else if (errCode2) { //Error trying to allocate out. Lets just put the bare string on there
			vector_putListBack(vecEx, arg, option, strlen(option));
			vector_putListBack(vecEx, arg, "\"", 1);
		}
	} else if(errCode == MYERR_ALLOCFAIL) {
		log_write(LEVEL_ERROR, "Failed to allocate more space for fg string");
		longjmp(jmpBuf, errCode);
	}
}

void barconfig_init(jmp_buf jmpBuf, void* obj, char* configDir) {
	vector_init(jmpBuf, obj, sizeof(char), 128);
	vector_putListBack(jmpBuf, obj, configDir, strlen(configDir)+1);
	return;
}

void barconfig_getArg(jmp_buf jmpBuf, void* obj, char* out, size_t outSize) {
	Vector args;
	vector_init(jmpBuf, &args, sizeof(char), 128);
	struct ConfigParser parser;
	struct ConfigParserEntry entry[] = {
		StringConfigEntry("bar:geometry", geometry, NULL),
		StringConfigEntry("bar:background", background, NULL),
		StringConfigEntry("bar:foreground", foreground, NULL),
	};
	cp_init(jmpBuf, &parser, entry);
	char* path = pathAppend(((Vector*)obj)->data, "bard.conf");
	cp_load(jmpBuf, &parser, path, &args);
	free(path);
	cp_kill(&parser);
	vector_putBack(jmpBuf, &args, "\0");

	size_t argLen = strlen(out);
	if(argLen + vector_size(&args) > outSize)
		longjmp(jmpBuf, MYERR_NOSPACE);
	strcpy(out + argLen, args.data);
}

void barconfig_kill(void* obj) {
	vector_kill(obj); //OBJ IS A VECTOR
}
