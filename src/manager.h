#ifndef MANAGER_H
#define MANAGER_H

#include <unistd.h>
#include <stdio.h>

struct bar_manager {
	int fd[2];
	pid_t pid;
	char* launchStr;
};

struct bar_manager manager_startBar(const char* executable, const char* argStr);
void manager_restartBar(struct bar_manager* manager);
void manager_exitBar(struct bar_manager* manager);
void manager_setOutput(struct bar_manager* manager, const char* output);

#endif
