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
#include <signal.h>
#include <stdio.h>
#include <wordexp.h>
#include <sys/wait.h>
#include <errno.h>
#include <assert.h>
#include "myerror.h"
#include "logger.h"

static pid_t run(char* command, int writefd) {
	assert(writefd > 0);
	assert(command != NULL);

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
		if(dup2(writefd, STDOUT_FILENO) == -1) {
			exit(1); //If dup2 fails in the forked process all hope is lost
		}
		execvp(wexp.we_wordv[0], wexp.we_wordv);
		exit(0);
	}
	wordfree(&wexp);
	return pid;
}

struct BufferData {
	Vector buffer;
	bool complete; //@MEM: We could probably save some memory here
	int rc;
};
struct Buffer {
	struct BufferData* data;
	int fd;
	int writefd;
	pid_t pid;
};
struct initPipeData {
	struct RunnerBuffer* buffers;
};
bool initPipe(void* elem, void* userdata) {
	struct Unit* unit = (struct Unit*)elem;
	struct initPipeData* data = (struct initPipeData*)userdata;

	if(unit->type != UNIT_RUNNING)
		return true;

	struct Buffer** val;
	JSLG(val, data->buffers->cbuff, unit->command);
	if(val == NULL) {
		struct Buffer* newBuf = calloc(sizeof(struct Buffer), 1);
		if(newBuf == NULL)
			THROW_NEW(false, "Failed allocating the command buffer");
		newBuf->data = calloc(sizeof(struct BufferData), 1);
		if(newBuf->data == NULL)
			THROW_NEW(false, "Failed allocating the command buffer data");
		newBuf->data->rc = 1;
		vector_init(&newBuf->data->buffer, sizeof(char), 128);
		PROP_THROW(false, "While starting pipe for %s", unit->name);

		int fd[2];
		pipe(fd);
		PROP_THROW(false, "While starting pipe for %s", unit->name);
		log_write(LEVEL_INFO, "The fd's for %s are %d and %d", unit->command, fd[0], fd[1]);
		newBuf->fd = fd[0];
		newBuf->writefd = fd[1];

		newBuf->pid = run(unit->command, newBuf->writefd);
		PROP_THROW(false, "While starting pipe for %s", unit->name);

		size_t commandLen = strlen(unit->command);
		if(commandLen > data->buffers->longestKey)
			data->buffers->longestKey = commandLen;

		JSLI(val, data->buffers->cbuff, unit->command);
		*val = newBuf;
		struct BufferData** newData;
		JLI(newData, data->buffers->ubuff, (Word_t)unit);
		*newData = newBuf->data;
		int rc;
		J1S(rc, data->buffers->owners, (Word_t)unit);
	} else {
		//Command already started. We just need to hook unit up to the running command
		(*val)->data->rc += 1;
		struct BufferData** val2;
		JLI(val2, data->buffers->ubuff, (Word_t)unit);
		*val2 = (*val)->data;
	}

	return true;
}

void runner_startPipes(struct RunnerBuffer* buffers, struct Units* units) {
	struct initPipeData data = {
		buffers,
	};
	vector_foreach(&units->left, initPipe, &data);
	VPROP_THROW("While starting runner pipes for left side");
	vector_foreach(&units->center, initPipe, &data);
	VPROP_THROW("While starting runner pipes for center");
	vector_foreach(&units->right, initPipe, &data);
	VPROP_THROW("While starting runner pipes for right side");
}

void runner_stopPipes(struct RunnerBuffer* buffers) {
	uint8_t* index = malloc((buffers->longestKey+1) * sizeof(char));
	index[0] = '\0';
	struct Buffer** val;
	JSLF(val, buffers->cbuff, index);
	while(val != NULL) {
		kill((*val)->pid, SIGTERM);
		log_write(LEVEL_INFO, "Waiting for %d to exit", (*val)->pid);
		waitpid((*val)->pid, NULL, 0);
		close((*val)->fd);
		log_write(LEVEL_INFO, "kill %s", index);
		JSLN(val, buffers->cbuff, index);
	}
	Word_t bytes;
	JSLFA(bytes, buffers->cbuff);
	J1FA(bytes, buffers->owners);
	free(index);
}

fd_set runner_getfds(struct RunnerBuffer* buffers) {
	fd_set set;
	FD_ZERO(&set);
	uint8_t* index = malloc((buffers->longestKey+1) * sizeof(char));
	index[0] = '\0';
	struct Buffer** val;
	JSLF(val, buffers->cbuff, index);
	while(val != NULL) {
		FD_SET((*val)->fd, &set);
		log_write(LEVEL_INFO, "fdset %s, fd: %d", index, (*val)->fd);
		JSLN(val, buffers->cbuff, index);
	}
	free(index);
	return set;
}

bool runner_ready(struct RunnerBuffer* buffers, fd_set* fdset, struct Unit* unit) {
	struct Buffer** val;
	JSLG(val, buffers->cbuff, (uint8_t*)unit->command);
	bool isset = FD_ISSET((*val)->fd, fdset);
	return isset;
}

void runner_read(struct RunnerBuffer* buffers, struct Unit* unit, char** const out) {
	assert(unit->type == UNIT_RUNNING);

	struct Buffer* buffer;
	{
		//Done early to bail on error
		struct Buffer** val;
		JSLG(val, buffers->cbuff, unit->command);
		if(val == NULL)
			VTHROW_NEW("No command started for %s", unit->command);
		buffer = *val;
	}
	struct BufferData* udata;
	struct BufferData** udataPtr;
	JLG(udataPtr, buffers->ubuff, (Word_t)unit);
	if(udataPtr == NULL)
		VTHROW_NEW("No command started for %s", unit->name);
	udata = *udataPtr;

	int rc;
	J1T(rc, buffers->owners, (Word_t)unit);
	if(rc == 1) {
		//We own this buffer, so update it
		//A sliding window implementation for searching for the delimeter
		size_t delimiterLen = strlen(unit->delimiter);
		char* window = malloc((delimiterLen+1) * sizeof(char));
		window[delimiterLen] = '\0';
		int n = read(buffer->fd, window, delimiterLen);
		buffer->data->complete = true;
		while(strstr(window, unit->delimiter) == NULL) {
			if(n == 0) {
				buffer->data->complete = false;
				break;
			}else if(n == -1) {
				VTHROW_NEW("Could not read from pipe: %s", strerror(errno));
			}
			vector_putBack(&buffer->data->buffer, window + delimiterLen-1); //Put last char onto the final string
			VPROP_THROW("While inserting the last char into the buffer %c", window + delimiterLen-1);
			memmove(window+1, window, delimiterLen-1); //Move everything over one
			n = read(buffer->fd, window, 1);
		}
		vector_putListBack(&buffer->data->buffer, window, delimiterLen); //Put the remaining window on there
		VPROP_THROW("While appending the window to the buffer %s", window);
		free(window);

		if(buffer->data->complete) {
			//We are done with this buffer, finish the old one off
			vector_putListBack(&buffer->data->buffer, "\0", 1);
			VPROP_THROW("While adding null terminator");

			//Copy the data to the output
			size_t listSize = vector_size(&buffer->data->buffer) * sizeof(char);
			*out = malloc(listSize);
			if(*out == NULL)
				VTHROW_NEW("Failed allocating space for the output buffer");
			memcpy(*out, buffer->data->buffer.data, listSize);
			buffer->data->rc -= 1;
			if(buffer->data->rc == 0) {
				vector_kill(&buffer->data->buffer);
				free(buffer->data);
			}

			//And get a new buffer ready
			buffer->data = calloc(sizeof(struct BufferData), 1);
			if(buffer->data == NULL)
				VTHROW_NEW("Failed allocating new buffer data struct");
			vector_init(&buffer->data->buffer, sizeof(char), 512);
			VPROP_THROW("While initiating the new buffer");
			buffer->data->rc = 1;
			*udataPtr = buffer->data;
		} else {
			*out = NULL;
		}
	}else{
		//We don't own this buffer, so just read if complete
		if(udata->complete) {
			size_t listSize = vector_size(&udata->buffer) * sizeof(char);
			*out = malloc(listSize);
			if(*out == NULL)
				VTHROW_NEW("Failed allocating space for the output buffer");
			memcpy(*out, udata->buffer.data, listSize);
			udata->rc -= 1;
			if(udata->rc == 0) {
				vector_kill(&udata->buffer);
				free(udata);
			}
			*udataPtr = buffer->data;
			buffer->data->rc += 1;
		}else{
			*out = NULL;
		}
	}
}