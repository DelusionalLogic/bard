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
#include "barconfig.h"
#include "unitcontainer.h"
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
#include "parser.h"
#include "fs.h"
#include "manager.h"
#include "gdbus/dbus.h"

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

struct Bard {
	struct WorkManager wm;
	struct RunnerBuffer buffer;
	struct Units units;
	struct FontList flist;
	struct Outputs output;
	struct Dbus dbus;
	struct bar_manager manager;
	struct Regex regexCache;
	struct FormatArray xcolorArr;
	//TODO: This should not be here
	char* barLaunchStr;
	char* barExecutable;
};

int freePtr(void* elem, void* userdata) {
	char** data = (char**) elem;
	free(*data);
	return 0;
}

void render(struct Bard* bard, const char* separator, int monitors) {
	//What to do about this? It can't be pipelined because then i might run more than once
	//per sleep
	char* out = out_format(&bard->output, &bard->units, monitors, separator);
	VPROP_THROW("While rendering the output");
	manager_setOutput(&bard->manager, out);
	free(out);
}

char* getInput(struct RunnerBuffer* buff, struct Unit* unit) {
	char* unitStr = NULL;
	if(unit->type == UNIT_RUNNING) {
		runner_read(buff, unit, &unitStr);
		ERROR_ABORT("While reading from running unit %s", unit->name);
	} else if (unit->type == UNIT_POLL) {
		unitexec_execUnit(unit, &unitStr);
		ERROR_ABORT("While executing from unit %s", unit->name);
	} else if (unit->type == UNIT_STATIC) {
		unitStr = calloc(1, 1); //Quite the hack.
	}
	return unitStr;
}

char* formatUnit(struct Unit* unit, char* unitStr, struct Regex* regexCache, struct FormatArray* xcolorArr) {
	struct FormatArray regexArr = {0};
	struct FormatArray fontArr = {0};
	const struct FormatArray *formatArr[] = {
		&regexArr,
		xcolorArr,
		&fontArr,
	};
	char* str;
	font_getArray(unit, &fontArr); //TODO: Cache this fro every unit
	ERROR_ABORT("Getting the fonts for %s", unit->name);
	regex_match(regexCache, unit, unitStr, &regexArr);
	ERROR_ABORT("Matching the compiled regex for %s", unit->name);
	if(unit->advancedFormat) {
		int exitCode = advformat_execute(unit->format, unit->compiledEnv, unit->lEnvKey, formatArr, sizeof(formatArr)/sizeof(struct FormatArray*), &str);
		ERROR_ABORT("While formatting for %s", unit->name);
		if(exitCode != 0) {
			unit->render = false;
		} else {
			/* Don't format whatever we got, since we aren't going to render it anyway */
			unit->render = true;
		}
	} else {
		log_write(LEVEL_INFO, "Preprocessed: %d, Name: %s", unit->isPreProcessed, unit->name);
		formatter_format(&unit->compiledFormat, formatArr, sizeof(formatArr)/sizeof(struct FormatArray*), &str);
		ERROR_ABORT("While simple-formatting unit: %s", unit->name);
	}
	formatarray_kill(&regexArr);
	formatarray_kill(&fontArr);
	return str;
}

void load_units(struct Units* units, char* confDir) {
	units_init(units);
	if(error_waiting())
		error_abort();

	units_load(units, confDir);
	if(error_waiting())
		error_abort();

	units_preprocess(units);
	if(error_waiting()) {
		error_print();
		exit(1);
	}
}

void load_mainConfig(struct Bard* bard, char** separator, int* monitors, char* confDir) {
	char* confPath = pathAppend(confDir, "bard.conf");
	dictionary* dict = iniparser_load(confPath);
	{
		const char* psep = iniparser_getstring(dict, "display:separator", " | ");
		Vector compiled;
		parser_compileStr(psep, &compiled);

		const struct FormatArray *const staticFormatArray[] = {
			&bard->xcolorArr,
		};

		formatter_format(&compiled, staticFormatArray, sizeof(staticFormatArray)/sizeof(struct FormatArray*), separator);
		ERROR_ABORT("While formatting separator");

		parser_freeCompiled(&compiled);
		*monitors = iniparser_getint(dict, "display:monitors", 1);
	}
	{
		font_createFontList(&bard->flist, &bard->units, confPath);
		ERROR_ABORT("While creating font list");
	}
	{
		Vector launch;
		//Make the lemonbar launch string
		vector_init(&launch, sizeof(char), 1024);
		ERROR_ABORT("While constructing lemonbar launch string");

		const char* executable = iniparser_getstring(dict, "bar:path", "lemonbar");
		if(executable == NULL) {
			ERROR_NEW("Missing executable in config, and default didn't work?");
			error_abort();
		}

		//vector_putListBack(&launch, executable, strlen(executable));
		ERROR_ABORT("While constructing lemonbar launch string");

		const struct FormatArray* xcolorPtr = &bard->xcolorArr;
		barconfig_getArgs(&launch, confPath, &xcolorPtr, 1);
		ERROR_ABORT("While constructing lemonbar launch string");
		font_getArg(&bard->flist, &launch);
		ERROR_ABORT("While constructing lemonbar launch string");
		vector_putListBack(&launch, "\0", 1);
		ERROR_ABORT("While constructing lemonbar launch string");


		//Lets load that bar! (Not really though)
		bard->barLaunchStr = vector_detach(&launch);
		bard->barExecutable = malloc(strlen(executable));
		strcpy(bard->barExecutable, executable);

		free(confPath);
		iniparser_freedict(dict);
	}

}

void startup(struct Bard *bard, char ** separator, char* confDir, int* monitors){
	load_units(&bard->units, confDir);

	load_mainConfig(bard, separator, monitors, confDir);

	runner_startPipes(&bard->buffer, &bard->units);
	ERROR_ABORT("While starting the processes");

	workmanager_init(&bard->wm, &bard->buffer, &bard->dbus);
	ERROR_ABORT("While starting the workmanager");
	workmanager_addUnits(&bard->wm, &bard->units);
	ERROR_ABORT("While adding units to the workmanager");

	regex_init(&bard->regexCache);
	ERROR_ABORT("While initializing regex matcher");
	regex_compile(&bard->regexCache, &bard->units);
	ERROR_ABORT("While compiling regexes");
}

void bard_shutdown(struct Bard* bard) {
	runner_stopPipes(&bard->buffer);
	ERROR_ABORT("While stopping child units");
	workmanager_kill(&bard->wm);
	ERROR_ABORT("While shutting down workmanager");
	units_free(&bard->units);
	ERROR_ABORT("While freeing unitcontainer");
	font_kill(&bard->flist);
	ERROR_ABORT("While freeing font list");

	out_kill(&bard->output);
	ERROR_ABORT("While freeing output buffers");
}


int main(int argc, char **argv)
{
	error_init();

	struct arguments arguments = {0};
	argp_parse(&argp, argc, argv, 0, 0, &arguments);

	if(arguments.configDir == NULL)
	{
		log_write(LEVEL_ERROR, "Config directory required");
		exit(0);
	}

	struct Bard bard = {0};

	dbus_start(&bard.dbus, "/dk/slashwin/bard/1"); //TODO: Make the last param matter

	struct XlibColor xColor;

	{
		xcolor_loadColors(&xColor);
		ERROR_ABORT("While loading xcolor");
		xcolor_formatArray(&xColor, &bard.xcolorArr);
		ERROR_ABORT("While constructing xcolor format array");
	}

	char* separator;
	int monitors;

	startup(&bard, &separator, arguments.configDir, &monitors);

	bard.manager = manager_startBar(bard.barExecutable, bard.barLaunchStr);

	{
		//Main loop
		bool oneUpdate = false;
		while(true) {
			union Work work;
			enum WorkType type = workmanager_next(&bard.wm, &bard.dbus, &work);
			ERROR_ABORT("While in main loop");
			if(type == WT_DBUS) {
				if(work.dbus->command == DC_RESTART) {
					manager_restartBar(&bard.manager);
					render(&bard, separator, monitors);
				} else if(work.dbus->command == DC_RELOAD) {
					bard_shutdown(&bard);
					startup(&bard, &separator, arguments.configDir, &monitors);
				}
				free(work.dbus);
			}
			if(type == WT_UNIT) {
				struct UnitWork unitWork = work.unit;
				size_t index = 0;
				struct Unit** unitptr = vector_getFirst(&unitWork.unitsReady, &index);
				while(unitptr != NULL) {
					struct Unit* unit = *unitptr;
					log_write(LEVEL_INFO, "[%ld] Processing %s (%s, %s)", time(NULL), unit->name, unit->command, TypeStr[unit->type]);

					/* Format the output for the bar */
					char* unitStr = getInput(&bard.buffer, unit);
					ERROR_ABORT("While getting input for %s", unit->name);

					if(unitStr != NULL) {
						oneUpdate = true;

						char* formatted = formatUnit(unit, unitStr, &bard.regexCache, &bard.xcolorArr);

						out_set(&bard.output, unit, formatted);
					}
					if(unitStr != NULL)
						free(unitStr);
					unitptr = vector_getNext(&unitWork.unitsReady, &index);
				}
				if(oneUpdate && !workmanger_waiting(&bard.wm)) {
					render(&bard, separator, monitors);
					oneUpdate = false;
				}
			}
		}

		workmanager_kill(&bard.wm);
		regex_kill(&bard.regexCache);
	}

	bard_shutdown(&bard);

	dbus_stop(&bard.dbus);

	manager_exitBar(&bard.manager);

	return 0;
}
