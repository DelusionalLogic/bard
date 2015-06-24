#define __USE_GNU
#include <unistd.h>
#include <fcntl.h>
#undef __USE_GNU
#include <stdlib.h>
#include "exec.h"

FILE* parExec(const char* file, char* const argv[], char* const envp[]) {
	int fds[2];
	pipe(fds);
	if(fork() == 0) {
		close(fds[0]);
		dup2((int)stdin, fds[1]);
		close(fds[1]);
		execvpe(file, argv, envp);
	}
	close(fds[1]);
	//Make the read end of the pipe async, This gives is a SIGIO when data is ready
	fcntl(fds[0], F_SETFL, O_NONBLOCK | O_ASYNC);
	return fdopen(fds[0], "r");
}
