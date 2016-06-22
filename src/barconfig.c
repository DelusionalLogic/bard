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
#include "formatter.h"
#include "fs.h"
#include "vector.h"
#include "configparser.h"
#include "parser.h"

struct cnfData{
	Vector* arg;
	const struct FormatArray **arrays;
	size_t arraysCnt;
};

static void geometry(struct cnfData* data, const char* option) {
	if(option == NULL)
		return;
	vector_putListBack(data->arg, " -g \"", 5);
	vector_putListBack(data->arg, option, strlen(option));
	vector_putListBack(data->arg, "\"", 1);
}

static void background(struct cnfData* data, const char* option) {
	if(option == NULL)
		return;

	vector_putListBack(data->arg, " -B \"", 5);
	char* out;

	Vector compiled;
	parser_compileStr(option, &compiled);
	VPROP_THROW("While compiling background string");

	formatter_format(&compiled, data->arrays, data->arraysCnt, &out);
	VPROP_THROW("While formatting background color: %s", option);

	vector_putListBack(data->arg, out, strlen(out));
	parser_freeCompiled(&compiled);
	free(out);
	vector_putListBack(data->arg, "\"", 1);
}

static void foreground(struct cnfData* data, const char* option) {
	if(option == NULL)
		return;

	vector_putListBack(data->arg, " -F \"", 5);
	char* out;

	Vector compiled;
	parser_compileStr(option, &compiled);
	VPROP_THROW("While compiling foreground string");

	formatter_format(&compiled, data->arrays, data->arraysCnt, &out);
	VPROP_THROW("While formatting foreground color: %s", option);

	vector_putListBack(data->arg, out, strlen(out));
	parser_freeCompiled(&compiled);
	free(out);
	vector_putListBack(data->arg, "\"", 1);

}

void barconfig_getArgs(Vector* arg, char* configFile, const struct FormatArray* arrays[], size_t arraysCnt) {
	struct cnfData data = {
		.arg = arg,
		.arrays = arrays,
		.arraysCnt = arraysCnt,
	};
	struct ConfigParser parser;
	struct ConfigParserEntry entry[] = {
		StringConfigEntry("bar:geometry", geometry, NULL),
		StringConfigEntry("bar:background", background, NULL),
		StringConfigEntry("bar:foreground", foreground, NULL),
		{.name = NULL},
	};
	cp_init(&parser, entry);
	VPROP_THROW("While constructing arguments for bar from the config");
	cp_load(&parser, configFile, &data);
	VPROP_THROW("While constructing arguments for bar from the config");
	cp_kill(&parser);
}
