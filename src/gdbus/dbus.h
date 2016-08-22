#ifndef DBUS_H
#define DBUS_H

#include <pthread.h>
#include <gio/gio.h>

enum DbusCommand {
	DC_RESTART,
};

struct DbusWork {
	enum DbusCommand command;
};

struct Dbus {
	pthread_t thread;
	GMainLoop* loop;
	int fd[2];
};

void dbus_start(struct Dbus* dbus, char* name);
void dbus_stop(struct Dbus* dbus);

int dbus_getfd(struct Dbus* dbus);

#endif
