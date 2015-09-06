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
#define _GNU_SOURCE
#include <config.h>
#include <signal.h>
#include <iniparser.h>
#include <stdio.h>
#include <strings.h>
#include <stdlib.h>
#include <argp.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>
#include <setjmp.h>

#include "myerror.h"
#include "xlib_color.h"
#include "strcolor.h"
#include "barconfig.h"
#include "pipestage.h"
#include "unitcontainer.h"
#include "map.h"
#include "unit.h"
#include "formatter.h"
#include "advancedformat.h"
#include "unitexecute.h"
#include "runner.h"
#include "configparser.h"
#include "font.h"
#include "workmanager.h"
#include "output.h"
#include "logger.h"
#include "vector.h"
#include "linkedlist.h"
#include "fs.h"

const char* argp_program_version = PACKAGE_STRING;
const char* argp_program_bug_address = PACKAGE_BUGREPORT;

static char doc[] = "bard";
static char args_doc[] = "";
static struct argp_option options[] = {
	{ "config", 'c', "config", 0, "Configuration directory."},
	{ 0 }
};

struct arguments
{
	char* configDir;
};

static error_t parse_opt(int key, char* arg, struct argp_state *state)
{
	struct arguments *args = state->input;

	switch(key)
	{
		case 'c':
			args->configDir = arg;
			break;
		default:
			return ARGP_ERR_UNKNOWN;
	}
	return 0;
}

static struct argp argp = {options, parse_opt, args_doc, doc, 0, 0, 0};

#define NUM_STAGES 10
struct PipeStage pipeline[NUM_STAGES] = {0};

int freePtr(void* elem, void* userdata) {
	char** data = (char**) elem;
	free(*data);
	return 0;
}

struct PatternMatch{
	size_t startPos;
	size_t endPos;
};

void colorize(jmp_buf jmpBuf, const char* str, char** out) {
	bool first = true;
	char* curStr = str;
	Vector vec;

	jmp_buf newEx;
	int errCode = setjmp(newEx);
	if(errCode == 0)
		vector_init(newEx, &vec, sizeof(char), 64);
	else if(errCode == MYERR_ALLOCFAIL) {
		log_write(LEVEL_ERROR, "Failed to allocate output string");
		longjmp(jmpBuf, errCode);
	}

	for(int i = 0; i < NUM_STAGES; i++) {
		struct PipeStage stage = pipeline[i];
		if(stage.enabled != true)
			continue;
		if(stage.colorString != NULL) {
			stage.colorString(jmpBuf, stage.obj, curStr, &vec);
			if(!first)
				free(curStr);
			first = false;
			curStr = vector_detach(&vec);
			jmp_buf newEx;
			int errCode = setjmp(newEx);
			if(errCode == 0)
				vector_init(newEx, &vec, sizeof(char), 64);
			else if(errCode == MYERR_ALLOCFAIL){
				log_write(LEVEL_ERROR, "Failed to allocate output string");
				longjmp(jmpBuf, errCode);
			}
		}
	}
	*out = curStr;
}

struct Output outputter;
FILE* bar;

bool executeUnit(jmp_buf jmpBuf, struct Unit* unit)
{
	log_write(LEVEL_INFO, "[%ld] Executing %s (%s, %s)", time(NULL), unit->name, unit->command, TypeStr[unit->type]);

	/* Format the output for the bar */
	for(int i = 0; i < NUM_STAGES; i++) {
		struct PipeStage stage = pipeline[i];
		if(stage.enabled != true)
			continue;
		if(stage.process != NULL) {
			jmp_buf procEx;
			int errCode = setjmp(procEx);
			if(errCode == 0) {
				if(!stage.process(procEx, stage.obj, unit))
					return false;
			} else {
				stage.enabled = false;
				longjmp(jmpBuf, errCode);
			}
		}
	}
	return true;
}

void render(jmp_buf jmpBuf) {
	//What to do about this? It can't be pipelined because then i might run more than once
	//per sleep
	char* out = out_format(jmpBuf, &outputter, NULL);
	fprintf(bar, "%s\n", out);
	fprintf(stdout, "%s\n", out);
	free(out);
	fflush(bar);
	fflush(stdout);
	return true;
}

int main(int argc, char **argv)
{
	struct arguments arguments = {0};
	argp_parse(&argp, argc, argv, 0, 0, &arguments);

	if(arguments.configDir == NULL)
	{
		log_write(LEVEL_ERROR, "Config directory required");
		exit(0);
	}

	jmp_buf pipeEx;
	int errCode = setjmp(pipeEx);
	if(errCode == 0) {
		//Setup pipeline
		pipeline[0] = barconfig_getStage(pipeEx);
		pipeline[1] = unitexec_getStage(pipeEx);
		pipeline[2] = runner_getStage(pipeEx);
		pipeline[3] = formatter_getStage(pipeEx);
		pipeline[4] = advFormatter_getStage(pipeEx);
		pipeline[5] = font_getStage(pipeEx);
		pipeline[6] = color_getStage(pipeEx);
	} else if(errCode == MYERR_ALLOCFAIL) {
		log_write(LEVEL_FATAL, "Couldn't allocate a stage");
		exit(MYERR_ALLOCFAIL);
	} else {
		log_write(LEVEL_WARNING, "Unknown error with a stage, error: %d", errCode);
	}

	struct Units units;

	jmp_buf loadEx;
	errCode = setjmp(loadEx);
	if(errCode == 0) {
		units_init(loadEx, &units);
		units_load(loadEx, &units, arguments.configDir);
	} else {
		log_write(LEVEL_ERROR, "Failed to load units: %d", errCode);
		exit(errCode);
	}

	//Initialize all stages in pipeline (This is where they load the configuration)
	for(int i = 0; i < NUM_STAGES; i++) {
		struct PipeStage stage = pipeline[i];
		if(stage.enabled != true)
			continue;
		if(stage.create != NULL) {
			jmp_buf stageEx;
			int errCode = setjmp(stageEx);
			if(errCode == 0) {
				stage.create(stageEx, stage.obj, arguments.configDir);
			} else {
				log_write(LEVEL_WARNING, "Unknown error creating pipe stage %d, error: %d", i, errCode);
				stage.enabled = false;
			}
		}
	}

	for(int i = 0; i < NUM_STAGES; i++) {
		struct PipeStage stage = pipeline[i];
		if(stage.enabled != true)
			continue;
		if(stage.addUnits != NULL) {
			jmp_buf stageEx;
			int errCode = setjmp(stageEx);
			if(errCode == 0) {
				stage.addUnits(stageEx, stage.obj, &units);
			} else {
				log_write(LEVEL_ERROR, "Unknown error while adding units to stage %d, error: %d", i, errCode);
				stage.enabled = false;
			}
		}
	}

	char* confPath = pathAppend(arguments.configDir, "bard.conf");
	dictionary* dict = iniparser_load(confPath);
	free(confPath);
	const char* executable = iniparser_getstring(dict, "bar:path", "lemonbar");

	char lBuff[2048];
	strcpy(lBuff, executable);
	iniparser_freedict(dict);
	for(int i = 0; i < NUM_STAGES; i++) {
		struct PipeStage stage = pipeline[i];
		if(stage.enabled != true)
			continue;
		if(stage.getArgs != NULL) {
			jmp_buf stageEx;
			int errCode = setjmp(stageEx);
			if(errCode == 0) {
				stage.getArgs(stageEx, stage.obj, lBuff, 2048);
			} else if(errCode == MYERR_ALLOCFAIL) {
				log_write(LEVEL_ERROR, "Couldn't prepare bar launch string. It would have been too long");
				exit(errCode);
			} else {
				log_write(LEVEL_WARNING, "Unknown error when constructing bar arg string in unit %d, error: %d", i, errCode);
			}
		}
	}

	//TODO: Where the hell does this belong?
	//Lets load that bar!
	log_write(LEVEL_INFO, "Starting %s", lBuff);
	bar = popen(lBuff, "w");

	jmp_buf outEx;
	errCode = setjmp(outEx);
	if(errCode == 0) {
		//Now lets run the units in a loop and write to bar
		out_init(outEx, &outputter, arguments.configDir); // Out is called from workmanager_run. It has to be ready before that is called
		out_addUnits(outEx, &outputter, &units);
	} else {
		log_write(LEVEL_FATAL, "Could not init outputter");
		exit(errCode);
	}

	struct WorkManager wm;
	jmp_buf manEx;
	errCode = setjmp(manEx);
	if(errCode == 0) {
		workmanager_init(manEx, &wm);

		int errCode = setjmp(manEx);
		if(errCode == 0) {
			workmanager_addUnits(manEx, &wm, &units);

			workmanager_run(manEx, &wm, executeUnit, render); //Blocks until the program should exit
		} else {
			log_write(LEVEL_FATAL, "Unknown error while executing workqueue %d", errCode);
			exit(errCode);
		}
	} else if(errCode == MYERR_ALLOCFAIL) {
		log_write(LEVEL_FATAL, "Allocation error while making the work queue");
		exit(errCode);
	} else {
		log_write(LEVEL_FATAL, "Unknown error while initializing workmanager");
	}

	workmanager_kill(&wm);

	out_kill(&outputter);

	for(int i = 0; i < NUM_STAGES; i++) {
		struct PipeStage stage = pipeline[i];
		if(stage.enabled != true)
			continue;
		if(stage.destroy != NULL)
			stage.destroy(stage.obj);
	}

	pclose(bar);

	return 0;
}
