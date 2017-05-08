#include "config.h"
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <gio/gio.h>
#include "logger.h"
#include "bard-generated.h"

#include <argp.h>

struct arg_global
{
	int verbosity;
};

void cmd_global(int argc, char**argv);
void cmd_config_path(struct argp_state* state);
void cmd_hide_unit(struct argp_state* state);
void cmd_show_unit(struct argp_state* state);

const char* argp_program_version = PACKAGE_STRING;
const char* argp_program_bug_address = PACKAGE_BUGREPORT;
error_t argp_err_exit_status = 1;

static struct argp_option opt_global[] =
{
	// { name, key, arg-name, flags, doc, group }

	{ "verbose", 'v', "level", OPTION_ARG_OPTIONAL, "Increase or set the verbosity level.", -1},

	{ "quiet", 'q', 0, 0, "Set verbosity to 0.", -1},

	// make -h an alias for --help
	{ 0 }
};

static char doc_global[] =
"\n"
"bard controller program."
"\v"
"Supported commands are:\n"
"  config-path    Show the config path.\n"
"  hide-unit      Hide a unit on a monitor.\n"
"  show-unit      Show a unit on a monitor."
;

static error_t parse_global(int key, char* arg, struct argp_state* state)
{
	struct arg_global* global = state->input;

	switch(key)
	{
		case 'v':
			if(arg)
				global->verbosity = atoi(arg);
			else
				global->verbosity++;
			log_write(LEVEL_INFO, "x: set verbosity to %d\n", global->verbosity);
			break;

		case 'q':
			log_write(LEVEL_INFO, "x: setting verbosity to 0\n");
			global->verbosity = 0;
			break;

		case ARGP_KEY_ARG:
			assert( arg );
			if(strcmp(arg, "config-path") == 0) {
				cmd_config_path(state);
			}else if(strcmp(arg, "hide-unit") == 0) {
				cmd_hide_unit(state);
			}else if(strcmp(arg, "show-unit") == 0) {
				cmd_show_unit(state);
			}else{
				argp_error(state, "%s is not a valid command", arg);
			}
			break;

		default:
			return ARGP_ERR_UNKNOWN;
	}

	return 0;
}

static struct argp argp =
{
	opt_global,
	parse_global,
	"[<cmd> [CMD-OPTIONS]]...",
	doc_global,
};

void cmd_global(int argc, char**argv)
{
	struct arg_global global =
	{
		1, /* default verbosity */
	};

	argp_parse(&argp, argc, argv, ARGP_IN_ORDER, NULL, &global);
}

/** Config-path Command **/

struct arg_config_path
{
	struct arg_global* global;
};

static struct argp_option opt_config_path[] =
{
	{ 0 }
};

static char doc_config_path[] = "";

	static error_t
parse_config_path(int key, char* arg, struct argp_state* state)
{
	struct arg_config_path* config_path = state->input;

	assert(config_path);
	assert(config_path->global);

	switch(key)
	{
		default:
			return ARGP_ERR_UNKNOWN;
	}

	return 0;
}

static struct argp argp_config_path =
{
	opt_config_path,
	parse_config_path,
	0,
	doc_config_path
};

void cmd_config_path(struct argp_state* state)
{
	struct arg_config_path config_path = { 0, };
	int    argc = state->argc - state->next + 1;
	char** argv = &state->argv[state->next - 1];

	char*  argv0 =  argv[0];

	config_path.global = state->input;

	argv[0] = malloc(strlen(state->name) + strlen(" config-path") + 1);
	if(!argv[0])
		argp_failure(state, 1, ENOMEM, 0);
	sprintf(argv[0], "%s config-path", state->name);

	argp_parse(&argp_config_path, argc, argv, ARGP_IN_ORDER, &argc, &config_path);

	free(argv[0]);
	argv[0] = argv0;

	state->next += argc - 1;

	GDBusConnection* connection = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, NULL);
	dbusBard* dbus = dbus_bard_proxy_new_sync(
			connection,
			G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START,
			"dk.slashwin.bard",
			"/dk/slashwin/bard",
			NULL,
			NULL
	);

	printf("%s\n", dbus_bard_get_config_path(dbus));

	return;
}

/** Hide-unit Command **/

struct arg_hide_unit
{
	struct arg_global* global;
	char* positionals[2];
};

static struct argp_option opt_hide_unit[] =
{
	{ 0 }
};

static char doc_hide_unit[] = "";

static error_t parse_hide_unit(int key, char* arg, struct argp_state* state)
{
	struct arg_hide_unit* hide_unit = state->input;

	assert(hide_unit);
	assert(hide_unit->global);

	switch(key)
	{
		case ARGP_KEY_ARG:
			if (state->arg_num >= 2)
			{
				argp_usage(state);
			}
			hide_unit->positionals[state->arg_num] = arg;
			break;
		case ARGP_KEY_END:
			if (state->arg_num < 2)
			{
				argp_usage (state);
			}
			break;
		default:
			return ARGP_ERR_UNKNOWN;
	}

	return 0;
}

static struct argp argp_hide_unit =
{
	opt_hide_unit,
	parse_hide_unit,
	"UNIT MONITOR",
	doc_hide_unit
};

void cmd_hide_unit(struct argp_state* state)
{
	struct arg_hide_unit hide_unit = { 0, };
	int    argc = state->argc - state->next + 1;
	char** argv = &state->argv[state->next - 1];

	char*  argv0 =  argv[0];

	hide_unit.global = state->input;

	argv[0] = malloc(strlen(state->name) + strlen(" hide-unit") + 1);
	if(!argv[0])
		argp_failure(state, 1, ENOMEM, 0);
	sprintf(argv[0], "%s hide-unit", state->name);

	argp_parse(&argp_hide_unit, argc, argv, ARGP_IN_ORDER, &argc, &hide_unit);

	free(argv[0]);
	argv[0] = argv0;

	state->next += argc - 1;

	GDBusConnection* connection = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, NULL);
	dbusBard* dbus = dbus_bard_proxy_new_sync(
			connection,
			G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START,
			"dk.slashwin.bard",
			"/dk/slashwin/bard",
			NULL,
			NULL
	);

	dbus_bard_call_disable_unit_sync(
			dbus,
			hide_unit.positionals[0],
			atoi(hide_unit.positionals[1]),
			NULL,
			NULL
	);

	return;
}

/** Show-unit Command **/

struct arg_show_unit
{
	struct arg_global* global;
	char* positionals[2];
};

static struct argp_option opt_show_unit[] =
{
	{ 0 }
};

static char doc_show_unit[] = "";

static error_t parse_show_unit(int key, char* arg, struct argp_state* state)
{
	struct arg_show_unit* show_unit = state->input;

	assert(show_unit);
	assert(show_unit->global);

	switch(key)
	{
		case ARGP_KEY_ARG:
			if (state->arg_num >= 2)
			{
				argp_usage(state);
			}
			show_unit->positionals[state->arg_num] = arg;
			break;
		case ARGP_KEY_END:
			if (state->arg_num < 2)
			{
				argp_usage (state);
			}
			break;
		default:
			return ARGP_ERR_UNKNOWN;
	}

	return 0;
}

static struct argp argp_show_unit =
{
	opt_show_unit,
	parse_show_unit,
	"UNIT MONITOR",
	doc_show_unit
};

void cmd_show_unit(struct argp_state* state)
{
	struct arg_show_unit show_unit = { 0, };
	int    argc = state->argc - state->next + 1;
	char** argv = &state->argv[state->next - 1];

	char*  argv0 =  argv[0];

	show_unit.global = state->input;

	argv[0] = malloc(strlen(state->name) + strlen(" show-unit") + 1);
	if(!argv[0])
		argp_failure(state, 1, ENOMEM, 0);
	sprintf(argv[0], "%s show-unit", state->name);

	argp_parse(&argp_hide_unit, argc, argv, ARGP_IN_ORDER, &argc, &show_unit);

	free(argv[0]);
	argv[0] = argv0;

	state->next += argc - 1;

	GDBusConnection* connection = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, NULL);
	dbusBard* dbus = dbus_bard_proxy_new_sync(
			connection,
			G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START,
			"dk.slashwin.bard",
			"/dk/slashwin/bard",
			NULL,
			NULL
	);

	dbus_bard_call_enable_unit_sync(
			dbus,
			show_unit.positionals[0],
			atoi(show_unit.positionals[1]),
			NULL,
			NULL
	);

	return;
}

/** Main **/

int main(int argc, char** argv)
{
	cmd_global(argc, argv);

	return 0;
}
