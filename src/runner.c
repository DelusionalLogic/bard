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
#include <assert.h>
#include "myerror.h"
#include "logger.h"

static int run(jmp_buf jmpBuf, char* command, int writefd) {
	assert(writefd > 0);
	assert(command != NULL);

	wordexp_t wexp;
	int err = wordexp(command, &wexp, WRDE_NOCMD);
	if(err == WRDE_CMDSUB) {
		wordfree(&wexp);
		longjmp(jmpBuf, MYERR_USERINPUTERR);
	}
	if(err != 0) {
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
		if(dup2(writefd, STDOUT_FILENO) == -1) {
			exit(1); //If dup2 fails in the forked process all hope is lost
		}
		execvp(wexp.we_wordv[0], wexp.we_wordv);
	}
	wordfree(&wexp);
	return 0;
}

struct Buffer {
	Vector buffer;
	bool complete;
	int fd;
	int writefd;
};
struct initPipeData {
	struct RunnerBuffer* buffers;
};
bool initPipe(jmp_buf jmpBuf, void* elem, void* userdata) {
	struct Unit* unit = (struct Unit*)elem;
	struct initPipeData* data = (struct initPipeData*)userdata;

	if(unit->type != UNIT_RUNNING)
		return true;

	struct Buffer** val;
	JSLG(val, data->buffers->buffers, unit->command);
	if(val == NULL) {
		struct Buffer* newBuf = calloc(sizeof(struct Buffer), 1);
		vector_init(jmpBuf, &newBuf->buffer, sizeof(char), 128);
		log_write(LEVEL_INFO, "Creating pipe for %s", unit->name);

		int fd[2];
		pipe(fd);
		log_write(LEVEL_INFO, "The new fd's are %d and %d", fd[0], fd[1]);
		newBuf->fd = fd[0];
		newBuf->writefd = fd[1];

		run(jmpBuf, unit->command, newBuf->writefd);

		size_t commandLen = strlen(unit->command) + 1;
		if(commandLen > data->buffers->longestKey)
			data->buffers->longestKey = commandLen;

		JSLI(val, data->buffers->buffers, unit->command);
		*val = newBuf;
		int rc;
		J1S(rc, data->buffers->owners, unit);
	}

	return true;
}

void runner_startPipes(jmp_buf jmpBuf, struct RunnerBuffer* buffers, struct Units* units) {
	jmp_buf runEx;
	int errCode = setjmp(runEx);
	if(errCode == 0) {
		struct initPipeData data = {
			buffers,
		};
		vector_foreach(runEx, &units->left, initPipe, &data);
		vector_foreach(runEx, &units->center, initPipe, &data);
		vector_foreach(runEx, &units->right, initPipe, &data);
	} else {
		longjmp(jmpBuf, errCode);
	}
}

fd_set runner_getfds(jmp_buf jmpBuf, struct RunnerBuffer* buffers) {
	fd_set set;
	FD_ZERO(&set);
	uint8_t* index = malloc(buffers->longestKey * sizeof(char));
	index[0] = '\0';
	struct Buffer** val;
	JSLF(val, buffers->buffers, index);
	while(val != NULL) {
		FD_SET((*val)->fd, &set);
		log_write(LEVEL_INFO, "fdset %s", index);
		JSLN(val, buffers->buffers, index);
	}
	free(index);
	return set;
}

bool runner_ready(jmp_buf jmpBuf, struct RunnerBuffer* buffers, fd_set* fdset, struct Unit* unit) {
	struct Buffer** val;
	JSLG(val, buffers->buffers, (uint8_t*)unit->command);
	bool isset = FD_ISSET((*val)->fd, fdset);
	return isset;
}

void runner_read(jmp_buf jmpBuf, struct RunnerBuffer* buffers, struct Unit* unit, char** const out) {
	assert(unit->type == UNIT_RUNNING);

	struct Buffer* buffer;
	{
		struct Buffer** val;
		JSLG(val, buffers->buffers, (uint8_t*)unit->command);
		if(val == NULL) {
			log_write(LEVEL_ERROR, "No command started for %s", unit->name);
			longjmp(jmpBuf, MYERR_BADFD);
		}
		buffer = *val;
	}

	int rc;
	J1T(rc, buffers->owners, (Word_t)unit);
	if(rc == 1) {
		//We own this buffer, so update it
		jmp_buf procEx;
		int errCode = setjmp(procEx);
		if(errCode == 0) {
			//A sliding window implementation for searching for the delimeter
			size_t delimiterLen = strlen(unit->delimiter);
			char* window = malloc((delimiterLen+1) * sizeof(char));
			window[delimiterLen] = '\0';
			int n = read(buffer->fd, window, delimiterLen);
			buffer->complete = true;
			while(strstr(window, unit->delimiter) == NULL) {
				if(n == 0) {
					buffer->complete = false;
					break;
				}else if(n == -1) {
					log_write(LEVEL_ERROR, "Could not read from pipe: %s", strerror(errno));
					longjmp(procEx, 0xDEADBEEF); //TODO: ERR CODE
				}
				vector_putBack(procEx, &buffer->buffer, window + delimiterLen-1); //Put last char onto the final string
				memmove(window+1, window, delimiterLen-1); //Move everything over one
				n = read(buffer->fd, window, 1);
			}
			vector_putListBack(procEx, &buffer->buffer, window, delimiterLen); //Put the remaining window on there
			free(window);

			if(buffer->complete) {
				vector_putListBack(jmpBuf, &buffer->buffer, "\0", 1);
				*out = vector_detach(&buffer->buffer);
				vector_init(jmpBuf, &buffer->buffer, sizeof(char), 512);
				return;
			}

			*out = NULL;
		} else {
			log_write(LEVEL_ERROR, "Error while retrieving input from running unit: %s", unit->name);
			longjmp(jmpBuf, errCode);
		}
	}else{
		//We don't own this buffer, so just read if complete
		if(buffer->complete) {
			*out = buffer->buffer.data;
		}else{
			*out = NULL;
		}
	}
}
