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
#include "runner.h"
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <wordexp.h>
#include <errno.h>
#include "myerror.h"
#include "unitcontainer.h"
#include "logger.h"

void runner_addUnits(jmp_buf, void* obj, struct Units* units);
bool runner_process(jmp_buf, void* obj, struct Unit* unit);

struct PipeStage runner_getStage(jmp_buf jmpBuf) {
	struct PipeStage stage;
	stage.enabled = true;
	stage.obj = NULL;
	stage.create = NULL;
	stage.addUnits = (void (*)(jmp_buf, void*, struct Units*))runner_addUnits;
	stage.getArgs = NULL;
	stage.colorString = NULL;
	stage.process = (bool (*)(jmp_buf, void*, struct Unit* unit)) runner_process;
	stage.destroy = NULL;
	return stage;
}

static int run(jmp_buf jmpBuf, struct Unit* unit) {
	if(unit->pipe == -1) {
		log_write(LEVEL_ERROR, "Trying to execute unit %s, but the pipe isn't open\n", unit->name);
		longjmp(jmpBuf, MYERR_BADFD);
	}

	wordexp_t wexp;
	int err = wordexp(unit->command, &wexp, WRDE_NOCMD);
	if(err == WRDE_CMDSUB) {
		log_write(LEVEL_ERROR, "Command substitution is disabled in %s", unit->name);
		wordfree(&wexp);
		longjmp(jmpBuf, MYERR_USERINPUTERR);
	}
	if(err != 0) {
		log_write(LEVEL_ERROR, "Error in command argument of unit %s", unit->name);
		if(err == WRDE_NOSPACE)
			wordfree(&wexp);
		longjmp(jmpBuf, MYERR_USERINPUTERR);
	}
	int pid = fork();
	if(pid == -1) {
		switch(errno) {
			case EAGAIN:
				log_write(LEVEL_ERROR, "Maximum number of processes started");
				wordfree(&wexp);
				longjmp(jmpBuf, MYERR_NOPROCESS);
				break;
			case ENOMEM:
				log_write(LEVEL_ERROR, "Not enough memory to fork process");
				wordfree(&wexp);
				longjmp(jmpBuf, MYERR_ALLOCFAIL);
				break;
			case ENOSYS:
				log_write(LEVEL_ERROR, "What are you trying to run this on?");
				break;
		}
	}
	if(pid == 0) {
		if(dup2(unit->writefd, STDOUT_FILENO) == -1) {
			exit(1); //If dup2 fails in the forked process all hope is lost
		}
		execvp(wexp.we_wordv[0], wexp.we_wordv);
	}
	wordfree(&wexp);
	return 0;
}

struct initPipeData {
};
bool initPipe(jmp_buf jmpBuf, void* elem, void* userdata) {
	struct Unit* unit = (struct Unit*)elem;
	struct initPipeData* data = (struct initPipeData*)userdata;

	if(unit->type != UNIT_RUNNING || unit->pipe != -1)
		return true;

	log_write(LEVEL_INFO, "Creating pipe for %s\n", unit->name);

	int fd[2];
	pipe(fd);
	log_write(LEVEL_INFO, "The new fd's are %d and %d\n", fd[0], fd[1]);
	unit->pipe = fd[0];
	unit->writefd = fd[1];

	run(jmpBuf, unit);
	return true;
}

void runner_addUnits(jmp_buf jmpBuf, void* obj, struct Units* units) {
	jmp_buf runEx;
	int errCode = setjmp(runEx);
	if(errCode == 0) {
		vector_foreach(runEx, &units->left, initPipe, NULL);
		vector_foreach(runEx, &units->center, initPipe, NULL);
		vector_foreach(runEx, &units->right, initPipe, NULL);
	} else {
		longjmp(jmpBuf, errCode);
	}
}
bool runner_process(jmp_buf jmpBuf, void* obj, struct Unit* unit) {
	if(unit->type != UNIT_RUNNING)
		return true;

	jmp_buf procEx;
	int errCode = setjmp(procEx);
	if(errCode == 0) {
		Vector str;
		vector_init(procEx, &str, sizeof(char), 1024);
		size_t delimiterLen = strlen(unit->delimiter);
		char* window = malloc((delimiterLen+1) * sizeof(char));
		window[delimiterLen] = '\0';
		int n = read(unit->pipe, window, delimiterLen);
		while(strstr(window, unit->delimiter) == NULL) {
			if(n == 0)
				break;
			if(n == -1) {
				log_write(LEVEL_ERROR, "Could not read from pipe: %s", strerror(errno));
				longjmp(procEx, 0xDEADBEEF); //TODO: ERR CODE
			}
			vector_putBack(procEx, &str, window + delimiterLen-1); //Put last char onto the final string
			memcpy(window+1, window, delimiterLen-1); //This should move everything over one
			n = read(unit->pipe, window, 1);
		}
		vector_putListBack(procEx, &str, window, delimiterLen+1); //Put the \0 on there too

		if(vector_size(&str) + unit->buffoff > UNIT_BUFFLEN) {
			longjmp(procEx, MYERR_NOSPACE);
		}
		strcpy(unit->buffer + unit->buffoff, str.data); //TODO: reverse str.data
		vector_kill(&str);
		if(n == 0) {
			unit->buffoff += vector_size(&str)-1;
			return false;
		}
		unit->buffoff = 0;
		return true;
	} else {
		log_write(LEVEL_ERROR, "Error while retrieving input from running unit: %s", unit->name);
		unit->buffer[0] = '\0';
		longjmp(jmpBuf, errCode);
	}
}
