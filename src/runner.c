#define _GNU_SOURCE
#include "runner.h"
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <wordexp.h>
#include <errno.h>
#include "unitcontainer.h"
#include "logger.h"

int runner_addUnits(void* obj, struct Units* units);
int runner_process(void* obj, struct Unit* unit);

struct PipeStage runner_getStage() {
	struct PipeStage stage;
	stage.error = 0;
	stage.enabled = true;
	stage.obj = NULL;
	stage.create = NULL;
	stage.addUnits = (int (*)(void*, struct Units*))runner_addUnits;
	stage.getArgs = NULL;
	stage.colorString = NULL;
	stage.process = (int (*)(void*, struct Unit* unit)) runner_process;
	stage.destroy = NULL;
	return stage;
}

static int run(struct Unit* unit) {
	if(unit->pipe == -1) {
		log_write(LEVEL_ERROR, "Trying to execute unit %s, but the pipe isn't open\n", unit->name);
		return 1;
	}

	wordexp_t wexp;
	int err = wordexp(unit->command, &wexp, WRDE_NOCMD);
	if(err == WRDE_CMDSUB) {
		log_write(LEVEL_ERROR, "Command substitution is disabled in %s", unit->name);
		wordfree(&wexp);
		return 1;
	}
	if(err != 0) {
		log_write(LEVEL_ERROR, "Error in command argument of unit %s", unit->name);
		if(err == WRDE_NOSPACE)
			wordfree(&wexp);
		return 1;
	}
	int pid = fork();
	if(pid == -1) {
		log_write(LEVEL_ERROR, "Couldn't create child process for unit %s", unit->name);
		return 2;
	}
	if(pid == 0) {
		dup2(unit->writefd, STDOUT_FILENO);
		execvp(wexp.we_wordv[0], wexp.we_wordv);
	}
	wordfree(&wexp);
	return 0;
}

struct initPipeData {
};
int initPipe(void* elem, void* userdata) {
	struct Unit* unit = (struct Unit*)elem;
	struct initPipeData* data = (struct initPipeData*)userdata;

	if(unit->type != UNIT_RUNNING || unit->pipe != -1)
		return 0;

	log_write(LEVEL_INFO, "Creating pipe for %s\n", unit->name);

	int fd[2];
	pipe(fd);
	log_write(LEVEL_INFO, "The new fd's are %d and %d\n", fd[0], fd[1]);
	unit->pipe = fd[0];
	unit->writefd = fd[1];

	run(unit);
	return 0;
}

int runner_addUnits(void* obj, struct Units* units) {
	int err = 0;
	err = vector_foreach(&units->left, initPipe, NULL);
	if(err)
		return err;
	err = vector_foreach(&units->center, initPipe, NULL);
	if(err)
		return err;
	err = vector_foreach(&units->right, initPipe, NULL);
	return err;
}
int runner_process(void* obj, struct Unit* unit) {
	if(unit->type != UNIT_RUNNING)
		return 0;

	Vector str;
	vector_init(&str, sizeof(char), 1024);
	size_t delimiterLen = strlen(unit->delimiter);
	char* window = malloc((delimiterLen+1) * sizeof(char));
	window[delimiterLen] = '\0';
	int n = read(unit->pipe, window, delimiterLen);
	while(strstr(window, unit->delimiter) == NULL) {
		if(n == 0)
			break;
		if(n == -1) {
			log_write(LEVEL_ERROR, "Could not read from pipe: %s", strerror(errno));
			return 2;
		}
		vector_putBack(&str, window + delimiterLen-1); //Put last char onto the final string
		memcpy(window+1, window, delimiterLen-1); //This should move everything over one
		n = read(unit->pipe, window, 1);
	}
	vector_putListBack(&str, window, delimiterLen+1); //Put the \0 on there too

	if(vector_size(&str) + unit->buffoff > UNIT_BUFFLEN) {
		log_write(LEVEL_ERROR, "Running string too long: %s", unit->name);
		return 1;
	}
	strcpy(unit->buffer + unit->buffoff, str.data); //TODO: reverse str.data
	vector_kill(&str);
	if(n == 0) {
		unit->buffoff += vector_size(&str)-1;
		return 1;
	}
	unit->buffoff = 0;
	return 0;
}
