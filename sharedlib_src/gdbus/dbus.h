#ifndef DBUS_H
#define DBUS_H

#include <pthread.h>
#include <gio/gio.h>

enum DbusCommand {
	DC_RESTART,
	DC_RELOAD,
	DC_DISABLEUNIT,
	DC_ENABLEUNIT,
};

struct EnDisWork {
	char* unitName;
	int monitor;
};

struct DbusWork {
	enum DbusCommand command;
	union {
		struct EnDisWork endis;
	};
};

struct Dbus {
	pthread_t thread;
	GMainLoop* loop;
	int fd[2];
	char* configPath;
};

void dbus_start(struct Dbus* dbus, char* name, char* configPath);
void dbus_stop(struct Dbus* dbus);

int dbus_getfd(struct Dbus* dbus);

#endif
