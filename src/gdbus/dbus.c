#include "dbus.h"
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "bard-generated.h"
#include "logger.h"
#include "myerror.h"

static gboolean on_handle_bar_reload(dbusBard* obj, GDBusMethodInvocation* inv, gpointer userdata) {
	struct Dbus* dbus = (struct Dbus*)userdata;
	log_write(LEVEL_INFO, "Dbus told us to restart the bar");
	log_write(LEVEL_INFO, "Dbus fd %d", dbus->fd[1]);

	struct DbusWork* work = malloc(sizeof(struct DbusWork));
	work->command = DC_RESTART;

	//Should be atomic according to POSIX spec, assuming that PIPE_BUF > 4
	write(dbus->fd[1], &work, sizeof(&work));

	dbus_bard_complete_bar_reload(obj, inv);

	return true;
}

static gboolean on_handle_reload(dbusBard* obj, GDBusMethodInvocation* inv, gpointer userdata) {
	struct Dbus* dbus = (struct Dbus*)userdata;
	log_write(LEVEL_INFO, "Dbus told us to reload");

	struct DbusWork* work = malloc(sizeof(struct DbusWork));
	work->command = DC_RELOAD;

	//Should be atomic according to POSIX spec, assuming that PIPE_BUF > 4
	write(dbus->fd[1], &work, sizeof(&work));

	dbus_bard_complete_reload(obj, inv);

	return true;
}

static gboolean on_handle_disable_unit(dbusBard* obj, GDBusMethodInvocation* inv, char* name, int32_t monitor, gpointer userdata) {
	struct Dbus* dbus = (struct Dbus*)userdata;

	log_write(LEVEL_INFO, "Dbus told us to disable %s, on %d", name, monitor);

	struct DbusWork* work = malloc(sizeof(struct DbusWork));
	work->command = DC_DISABLEUNIT;
	work->endis.unitName = malloc(strlen(name) * sizeof(char));
	strcpy(work->endis.unitName, name);
	work->endis.monitor = monitor;

	//Should be atomic according to POSIX spec, assuming that PIPE_BUF > 4
	write(dbus->fd[1], &work, sizeof(&work));

	dbus_bard_complete_disable_unit(obj, inv);

	return true;
}

static gboolean on_handle_enable_unit(dbusBard* obj, GDBusMethodInvocation* inv, char* name, int32_t monitor, gpointer userdata) {
	struct Dbus* dbus = (struct Dbus*)userdata;

	log_write(LEVEL_INFO, "Dbus told us to enable %s, on %d", name, monitor);

	struct DbusWork* work = malloc(sizeof(struct DbusWork));
	work->command = DC_ENABLEUNIT;
	work->endis.unitName = malloc(strlen(name) * sizeof(char));
	strcpy(work->endis.unitName, name);
	work->endis.monitor = monitor;

	//Should be atomic according to POSIX spec, assuming that PIPE_BUF > 4
	write(dbus->fd[1], &work, sizeof(&work));

	dbus_bard_complete_enable_unit(obj, inv);

	return true;
}

static void on_bus_aquired(GDBusConnection* conn, const gchar* name, gpointer userdata) {
	struct Dbus* dbus = (struct Dbus*)userdata;
	dbusBard* bardbus = dbus_bard_skeleton_new();
	GError* error = NULL;
	if (!g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON (bardbus),
				conn,
				"/dk/slashwin/bard",
				&error))
	{
		log_write(LEVEL_ERROR, "Failed exporting skeleton: %s", error->message);
		g_error_free(error);
		return;
	}

	g_signal_connect(G_DBUS_INTERFACE_SKELETON(bardbus),
			"handle-bar-reload",
			G_CALLBACK(on_handle_bar_reload),
			userdata);

	g_signal_connect(G_DBUS_INTERFACE_SKELETON(bardbus),
			"handle-reload",
			G_CALLBACK(on_handle_reload),
			userdata);

	g_signal_connect(G_DBUS_INTERFACE_SKELETON(bardbus),
			"handle-disable-unit",
			G_CALLBACK(on_handle_disable_unit),
			userdata);

	g_signal_connect(G_DBUS_INTERFACE_SKELETON(bardbus),
			"handle-enable-unit",
			G_CALLBACK(on_handle_enable_unit),
			userdata);
}

static void on_name_aquired(GDBusConnection* conn, const gchar* name, gpointer userdata) {
	log_write(LEVEL_INFO, "Now taking calls on the dbus");
}

static void on_name_lost(GDBusConnection* conn, const gchar* name, gpointer userdata) {
	log_write(LEVEL_ERROR, "Lost dbus name");
}

static void* workThread(void* userdata) {
	struct Dbus* dbus = (struct Dbus*)userdata;

	dbus->loop = g_main_loop_new(NULL, false);

	guint owner_id = g_bus_own_name(G_BUS_TYPE_SESSION,
			"dk.slashwin.bard",
			G_BUS_NAME_OWNER_FLAGS_ALLOW_REPLACEMENT,
			on_bus_aquired,
			on_name_aquired,
			on_name_lost,
			userdata,
			NULL);

	g_main_loop_run(dbus->loop);
	g_bus_unown_name(owner_id);
	return NULL;
}

void dbus_start(struct Dbus* dbus, char* name) {
	pipe(dbus->fd);
	if(pthread_create(&dbus->thread, NULL, workThread, dbus) != 0)
		VTHROW_NEW("Failed creating dbus thread");
		return;
}

void dbus_stop(struct Dbus* dbus) {
	g_main_loop_quit(dbus->loop);
}

int dbus_getfd(struct Dbus* dbus) {
	return dbus->fd[0];
}
