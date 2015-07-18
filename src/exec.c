#define __USE_GNU
#define _GNU_SOURCE
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include "exec.h"

int parExec(const char* file, int out, char* const argv[], char* const envp[]) {
	if(fork() == 0) {
		dup2(out, STDOUT_FILENO);
		execvpe(file, argv, envp);
	}
	return 0;
}
