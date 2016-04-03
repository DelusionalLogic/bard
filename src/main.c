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

struct Units units;
struct Outputs outputs;
FILE* bar;


void render(jmp_buf jmpBuf, const char* separator) {
	//What to do about this? It can't be pipelined because then i might run more than once
	//per sleep
	char* out = out_format(jmpBuf, &outputs, &units, 1, separator);
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

	struct FontList flist = {0};

	jmp_buf loadEx;
	int errCode = setjmp(loadEx);
	if(errCode == 0) {
		units_init(loadEx, &units);
		units_load(loadEx, &units, arguments.configDir);
	} else {
		log_write(LEVEL_ERROR, "Failed to load units: %d", errCode);
		exit(errCode);
	}

	struct Regex regexCache;
	struct WorkManager wm;
	struct RunnerBuffer buff = {0};

	struct XlibColor xColor;

	struct FormatArray xcolorArr = {0};
	struct FormatArray regexArr = {0};
	struct FormatArray fontArr = {0};

	const struct FormatArray *formatArr[] = {
		&regexArr,
		&xcolorArr,
		&fontArr,
	};
	{
		jmp_buf excep;
		int errCode = setjmp(excep);
		if(errCode == 0) {
			xcolor_loadColors(excep, &xColor);
			xcolor_formatArray(excep, &xColor, &xcolorArr);
		} else {
			log_write(LEVEL_FATAL, "While initializing xcolor array (%d)", errCode);
			exit(errCode);
		}
	}


	char* separator;
	{

		char* confPath = pathAppend(arguments.configDir, "bard.conf");
		dictionary* dict = iniparser_load(confPath);
		{
			jmp_buf excep;
			int errCode = setjmp(excep);
			if(errCode == 0) {
				const char* psep = iniparser_getstring(dict, "display:separator", " | ");
				formatter_format(excep, psep, &formatArr[1], 1, &separator);
			} else {
				log_write(LEVEL_FATAL, "While reading/formatting seperator (%d)", errCode);
				exit(errCode);
			}
		}
		{
			jmp_buf excep;
			int errCode = setjmp(excep);
			if(errCode == 0) {
				font_createFontList(excep, &flist, &units, confPath);
			} else {
				log_write(LEVEL_FATAL, "While creating the fonst list (%d)", errCode);
				exit(errCode);
			}
		}
		{
			Vector launch;
			jmp_buf launchEx;
			int errCode = setjmp(launchEx);
			if(errCode == 0) {
				//Make the lemonbar launch string
				vector_init(launchEx, &launch, sizeof(char), 1024);

				const char* executable = iniparser_getstring(dict, "bar:path", "lemonbar");

				vector_putListBack(launchEx, &launch, executable, strlen(executable));

				iniparser_freedict(dict);
				jmp_buf stageEx;
				int errCode = setjmp(stageEx);
				if(errCode == 0) {
					struct FormatArray* xcolorPtr = &xcolorArr;
					barconfig_getArgs(stageEx, &launch, confPath, &xcolorPtr, 1);
					font_getArg(stageEx, &flist, &launch);
					vector_putListBack(stageEx, &launch, "\0", 1);
				} else {
					log_write(LEVEL_ERROR, "While constructing launch string (%d)", errCode);
				}

				free(confPath);

				//TODO: Where the hell does this belong?
				//Lets load that bar!
				log_write(LEVEL_INFO, "lemonbar launch string: %s", launch.data);
				bar = popen(launch.data, "w");
				vector_kill(&launch);

			} else {
				log_write(LEVEL_FATAL, "Couldn't allocate space for launch string");
				exit(errCode);
			}
		}
	}

	{
		jmp_buf excep;
		int errCode = setjmp(excep);
		if(errCode == 0) {
			runner_startPipes(excep, &buff, &units);
			workmanager_init(excep, &wm, &buff);
			workmanager_addUnits(excep, &wm, &units);
		} else {
			log_write(LEVEL_FATAL, "While initializing workmanager (%d)", errCode);
			exit(errCode);
		}
	}

	{
		jmp_buf excep;
		int errCode = setjmp(excep);
		if(errCode == 0) {
			runner_startPipes(excep, &buff, &units);
		} else {
			log_write(LEVEL_FATAL, "While starting running units (%d)", errCode);
			exit(errCode);
		}
	}

	{
		jmp_buf excep;
		int errCode = setjmp(excep);
		if(errCode == 0) {
			regex_init(&regexCache);
			regex_compile(excep, &regexCache, &units);
		} else {
			log_write(LEVEL_FATAL, "While initializing/compiling regex (%d)", errCode);
			exit(errCode);
		}
	}

	{
		jmp_buf manEx;
		errCode = setjmp(manEx);
		if(errCode == 0) {
			//Main loop
			while(true) {
				struct Unit* unit = workmanager_next(manEx, &wm);
				log_write(LEVEL_INFO, "[%ld] Processing %s (%s, %s)", time(NULL), unit->name, unit->command, TypeStr[unit->type]);

				/* Format the output for the bar */
				{
					char* unitStr;

					jmp_buf procEx;
					int errCode = setjmp(procEx);
					if(errCode == 0) {
						if(unit->type == UNIT_RUNNING) {
							runner_read(procEx, &buff, unit, &unitStr);
						} else if (unit->type == UNIT_POLL) {
							unitexec_execUnit(procEx, unit, &unitStr);
						} else if (unit->type == UNIT_STATIC) {
							unitStr = calloc(1, 1);
						}
						log_write(LEVEL_INFO, "Unit: %s had command output: %s", unit->name, unitStr);
						if(unitStr != NULL) {
							char* str;
							font_getArray(manEx, unit, &fontArr);
							regex_match(manEx, &regexCache, unit, unitStr, &regexArr);
							if(unit->advancedFormat) {
								advformat_execute(manEx, unit->format, formatArr, sizeof(formatArr)/sizeof(struct FormatArray*), &str);
								char* str2;
								formatter_format(manEx, str, formatArr, sizeof(formatArr)/sizeof(struct FormatArray*), &str2);
								free(str);
								str = str2;
							} else {
								formatter_format(manEx, unit->format, formatArr, sizeof(formatArr)/sizeof(struct FormatArray*), &str);
							}
							formatarray_kill(manEx, &regexArr);
							formatarray_kill(manEx, &fontArr);
							log_write(LEVEL_INFO, "Unit -> %s, str -> %s", unit->name, str);
							out_set(manEx, &outputs, unit, str);
						}
						if(!workmanger_waiting(manEx, &wm)) {
							render(manEx, separator);
						}
						free(unitStr);
					} else {
						longjmp(manEx, errCode);
					}
				}
			}
		} else {
			log_write(LEVEL_FATAL, "While in main loop (%s, %d)", strerror(errCode), errCode);
			exit(errCode);
		}

		workmanager_kill(&wm);
		regex_kill(&regexCache);
	}

	out_kill(&outputs);

	pclose(bar);

	return 0;
}
