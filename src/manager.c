#include "manager.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <wordexp.h>
#include <errno.h>
#include <assert.h>
#include "logger.h"
#include "myerror.h"

static pid_t run(char* command, int fd[2]) {
	assert(command != NULL);

	int fdin[2];
	int fdout[2];

	pipe(fdin);
	pipe(fdout);

	wordexp_t wexp;
	int err = wordexp(command, &wexp, WRDE_NOCMD);
	if(err == WRDE_CMDSUB) {
		wordfree(&wexp);
		THROW_NEW(0, "Failed performing shell expansion on %s", command);
	}
	if(err != 0) {
		if(err == WRDE_NOSPACE)
			wordfree(&wexp);
		THROW_NEW(0, "Failed performing shell expansion on %s", command);
	}
	int pid = fork();
	if(pid == -1) {
		switch(errno) {
			case EAGAIN:
				wordfree(&wexp);
				THROW_NEW(0, "Maximum number of processes started");
				break;
			case ENOMEM:
				wordfree(&wexp);
				THROW_NEW(0, "Not enough memory to fork process");
				break;
			case ENOSYS:
				wordfree(&wexp);
				THROW_NEW(0, "What are you trying to run this on?");
				break;
		}
	}
	if(pid == 0) {
		if(dup2(fdout[1], STDOUT_FILENO) == -1) {
			exit(1); //If dup2 fails in the forked process all hope is lost
		}
		if(dup2(fdin[0], STDIN_FILENO) == -1) {
			exit(1); //If dup2 fails in the forked process all hope is lost
		}
		execvp(wexp.we_wordv[0], wexp.we_wordv);
	}
	//Actually remember to close the unused file descriptors in the parent
	close(fdout[1]);
	close(fdin[0]);

	wordfree(&wexp);
	fd[0] = fdout[0];
	fd[1] = fdin[1];
	return pid;
}

struct bar_manager manager_startBar(const char* executable, const char* argStr) {
	struct bar_manager manager;
	size_t outLen = strlen(executable) + strlen(argStr) + 1;
	manager.launchStr = malloc(outLen * sizeof(char));
	strcpy(manager.launchStr, executable);
	strcpy(manager.launchStr + strlen(executable), argStr);
	log_write(LEVEL_INFO, "lemonbar launch string: %s", manager.launchStr);
	manager.pid = run(manager.launchStr, manager.fd);
	PROP_THROW(manager, "While starting bar");
	return manager;
}

void manager_restartBar(struct bar_manager* manager) {
	manager_exitBar(manager);
	manager->pid = run(manager->launchStr, manager->fd);
	VPROP_THROW("While restarting bar");
}

void manager_exitBar(struct bar_manager* manager) {
	if(close(manager->fd[1]))
		VTHROW_NEW("Failed closing file descriptor");
	if(close(manager->fd[0]))
		VTHROW_NEW("Failed closing file descriptor");
	kill(manager->pid, SIGTERM);
}

void manager_setOutput(struct bar_manager* manager, const char* output) {
	dprintf(manager->fd[1], "%s\n", output);
	fsync(manager->fd[1]);
}
