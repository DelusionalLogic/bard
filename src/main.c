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

#include "regex.h"
#include "myerror.h"
#include "xlib_color.h"
#include "strcolor.h"
#include "barconfig.h"
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

int freePtr(void* elem, void* userdata) {
	char** data = (char**) elem;
	free(*data);
	return 0;
}

struct PatternMatch{
	size_t startPos;
	size_t endPos;
};

struct Output outputter;
FILE* bar;

bool executeUnit(jmp_buf jmpBuf, struct Unit* unit)
{
	log_write(LEVEL_INFO, "[%ld] Executing %s (%s, %s)", time(NULL), unit->name, unit->command, TypeStr[unit->type]);

	/* Format the output for the bar */
	{
		char* unitStr;

		jmp_buf procEx;
		int errCode = setjmp(procEx);
		if(errCode == 0) {
			if(!unitexec_execUnit(procEx, unit, &unitStr)) {
				return false;
			}
		} else {
			longjmp(jmpBuf, errCode);
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

	struct Units units;

	jmp_buf loadEx;
	int errCode = setjmp(loadEx);
	if(errCode == 0) {
		units_init(loadEx, &units);
		units_load(loadEx, &units, arguments.configDir);
	} else {
		log_write(LEVEL_ERROR, "Failed to load units: %d", errCode);
		exit(errCode);
	}

	{
		Vector launch;
		jmp_buf launchEx;
		int errCode = setjmp(launchEx);
		if(errCode == 0) {
			vector_init(launchEx, &launch, sizeof(char), 1024);

			//Make the lemonbar launch string
			char* confPath = pathAppend(arguments.configDir, "bard.conf");
			dictionary* dict = iniparser_load(confPath);
			const char* executable = iniparser_getstring(dict, "bar:path", "lemonbar");
			vector_putListBack(launchEx, &launch, executable, strlen(executable));

			iniparser_freedict(dict);
			jmp_buf stageEx;
			int errCode = setjmp(stageEx);
			if(errCode == 0) {
				barconfig_getArgs(stageEx, &launch, confPath);
			} else {
				log_write(LEVEL_ERROR, "Unknown error when constructing bar arg string, error: %d", errCode);
			}

			free(confPath);

			//TODO: Where the hell does this belong?
			//Lets load that bar!
			log_write(LEVEL_INFO, "Starting %s", launch.data);
			bar = popen(launch.data, "w");
			vector_kill(&launch);

		} else {
			log_write(LEVEL_FATAL, "Couldn't allocate space for launch string");
			exit(errCode);
		}
	}

	jmp_buf outEx;
	errCode = setjmp(outEx);
	if(errCode == 0) {
		out_init(outEx, &outputter, arguments.configDir); // Out is called from workmanager_run. It has to be ready before that is called
		out_addUnits(outEx, &outputter, &units);
	} else {
		log_write(LEVEL_FATAL, "Could not init outputter");
		exit(errCode);
	}

	{
		struct Regex regexCache;
		regex_init(&regexCache);
		struct FormatArray regexArr = {0};

		jmp_buf regexEx;
		int errCode = setjmp(regexEx);
		if(errCode == 0) {
			regex_compile(regexEx, &regexCache, &units);
			regex_match(regexEx, &regexCache, vector_get(regexEx, &units.left, 0), NULL, &regexArr);
			char* str;
			formatter_format(regexEx, "$regex[a]hello $regex[a]world", &regexArr, 1, &str);
			log_write(LEVEL_INFO, "STR STR: %s", str);
			free(str);
		} else {
			log_write(LEVEL_FATAL, "Could not regex");
			exit(errCode);
		}

		regex_kill(&regexCache);
	}

	{
		struct XlibColor xColor;
		struct FormatArray xcolorArr = {0};

		jmp_buf regexEx;
		int errCode = setjmp(regexEx);
		if(errCode == 0) {
			xcolor_loadColors(regexEx, &xColor);
			xcolor_formatArray(regexEx, &xColor, vector_get(regexEx, &units.left, 0), &xcolorArr);
			char* str;
			formatter_format(regexEx, "$xcolor[black]hello $xcolor[white]world", &xcolorArr, 1, &str);
			log_write(LEVEL_INFO, "STR STR: %s", str);
			free(str);
		} else {
			log_write(LEVEL_FATAL, "Could not color");
			exit(errCode);
		}
	}

	struct WorkManager wm;
	jmp_buf manEx;
	errCode = setjmp(manEx);
	if(errCode == 0) {
		struct RunnerBuffer buff;
		runner_startPipes(manEx, &buff, &units);
		workmanager_init(manEx, &wm, &buff);

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

	pclose(bar);

	return 0;
}
