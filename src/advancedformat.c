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
#include "advancedformat.h"
#include <regex.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <unistd.h>
#include <wait.h>
#include <wordexp.h>
#include "formatter.h"
#include "myerror.h"
#include "logger.h"

int advformat_execute(jmp_buf jmpBuf, char* format, const struct FormatArray* fmtArrays[], size_t fmtLen, char** out) {
	Vector args;
	vector_init(jmpBuf, &args, sizeof(char*), 4);
	char* null = NULL;
	size_t cmdlen = 0;
	char* ch = format;
	//Get length of initial command
	while(*ch != ' ' && *ch != '\0') {
		cmdlen++;
		ch++;
	}
	{
		//Extract initial command and do shell expansion
		char* cmd = malloc(cmdlen + 1);
		memcpy(cmd, format, cmdlen);
		cmd[cmdlen] = '\0';
		wordexp_t p;
		wordexp(cmd, &p, 0);
		Vector newCmd;
		vector_init(jmpBuf, &newCmd, sizeof(char), 64);
		for(size_t i = 0; i < p.we_wordc; i++) {
			vector_putListBack(jmpBuf, &newCmd, p.we_wordv[i], strlen(p.we_wordv[i])+1);
		}
		wordfree(&p);
		free(cmd);
		cmd = vector_detach(&newCmd);
		vector_putBack(jmpBuf, &args, &cmd);
	}
	//Read all the following parameters and extract file formatting
	while(true) {
		while(*ch == ' ' && *ch != '\0') {
			ch++;
		}
		if(*ch == '\0')
			break;
		char* start = ch;
		while(*ch != ' ' && *ch != '\0') {
			ch++;
		}
		char* arg = malloc((ch-start) + 1);
		memcpy(arg, start, ch-start);
		arg[ch-start] = '\0';
		char* str;
		//Expand bard variables
		formatter_format(jmpBuf, arg, fmtArrays, fmtLen, &str);
		free(arg);
		vector_putBack(jmpBuf, &args, &str);

		if(*ch == '\0')
			break;
	}
	//Add null ptr to satisfy execv
	vector_putBack(jmpBuf, &args, &null);

	int pipefd[2];
	pipe(pipefd);

	//Start the child
	int pid = fork();
	if(pid == 0) {
		close(pipefd[0]);
		dup2(pipefd[1], STDOUT_FILENO);
		execvp(*(char**)vector_get(jmpBuf, &args, 0), (char**)args.data);
		exit(0);
	}

	close(pipefd[1]);
	FILE* output = fdopen(pipefd[0], "r");

	//Free the arguments
	{
		int index;
		char** arg = vector_getFirst(jmpBuf, &args, &index);
		while(arg != NULL) {
			if(*arg != NULL)
				free(*arg);
			arg = vector_getNext(jmpBuf, &args, &index);
		}
	}
	vector_kill(&args);

	Vector buff;
	vector_init(jmpBuf, &buff, sizeof(char), 1024);
	ssize_t readLen;

	/* Read output */
	char chunk[1024];
	while((readLen = fread(chunk, 1, 1024, output)) > 0){
		log_write(LEVEL_INFO, "Read: %d", readLen);
		vector_putListBack(jmpBuf, &buff, chunk, readLen);
	}
	if(ferror(output)) {
		longjmp(jmpBuf, errno);
	}

	//Remove trailing newline
	if(buff.size > 0 && buff.data[buff.size-1] == '\n')
		buff.data[buff.size-1] = '\0';
	else
		vector_putListBack(jmpBuf, &buff, "\0", 1);

	int status;
	waitpid(pid, &status, 0);
	int exitCode = WEXITSTATUS(status);
	close(pipefd[0]);
	log_write(LEVEL_INFO, "Done: %d -> %s", exitCode, buff.data);
	*out = vector_detach(&buff);
	return exitCode;
}
