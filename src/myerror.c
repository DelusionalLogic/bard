#include "myerror.h"
#include <pthread.h>
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include "vector.h"
#include "logger.h"

struct Error {
	char* message;
};

static pthread_key_t errKey;
static pthread_once_t key_once = PTHREAD_ONCE_INIT;

static void freeElems(Vector* errList) {
	size_t index;
	struct Error* err = vector_getFirst(errList, &index);
	while(err != NULL) {
		free(err->message);
		err = vector_getNext(errList, &index);
	}
}

static void killKey(void* val) {
	assert(val != NULL);
	Vector* errList = (Vector*)val;
	freeElems(errList);
	vector_kill(errList);
	free(errList);
}

static void makeKey() {
	pthread_key_create(&errKey, killKey);
}

static struct Error makeErr(char* file, int line, char* format, va_list args) {
	struct Error err = {0};

	size_t prefixLen = snprintf(NULL, 0, "%s:%d: ", file, line);

	va_list lenArgs;
	va_copy(lenArgs, args); //Copy to save the original args

	size_t errLen = vsnprintf(NULL, 0, format, lenArgs);

	err.message = malloc((prefixLen + errLen + 1) * sizeof(char));
	if(err.message == NULL) {
		log_write(LEVEL_FATAL, "I couldn't allocate space for the error message. Damn it");
		exit(1);
	}

	sprintf(err.message, "%s:%d: ", file, line);
	vsprintf(err.message + prefixLen, format, args);
	return err;
}

void error_init() {
	pthread_once(&key_once, makeKey);
}

void error_new(char* file, int line, char* format, ...) {
	Vector* ptr = (Vector*)pthread_getspecific(errKey);
	if(ptr != NULL) {
		log_write(LEVEL_FATAL, "Error already in progress");
		exit(1);
	}
	ptr = malloc(sizeof(Vector));
	if(ptr == NULL) {
		//We aren't going to free the ptr since we will exit anyway. Let the os handle it.
		log_write(LEVEL_FATAL, "I couldn't allocate a vector for the error messages. That really sucks");
		exit(1);
	}
	vector_init(ptr, sizeof(struct Error), 5);
	if(error_waiting()) {
		log_write(LEVEL_FATAL, "I couldn't initialize the error vector");
		exit(1);
	}

	pthread_setspecific(errKey, ptr);

	va_list args;
	va_start(args, format);
	struct Error err = makeErr(file, line, format, args);
	va_end(args);
	vector_putBack_assert(ptr, &err);
}

void error_append(char* file, int line, char* format, ...) {
	Vector* ptr = (Vector*)pthread_getspecific(errKey);
	if(ptr == NULL) {
		log_write(LEVEL_FATAL, "No error to append to");
		exit(1);
	}

	va_list args;
	va_start(args, format);
	struct Error err = makeErr(file, line, format, args);
	va_end(args);
	vector_putBack_assert(ptr, &err);
}

bool error_waiting() {
	Vector* ptr = (Vector*)pthread_getspecific(errKey);
	return ptr != NULL;
}

void error_eat() {
	Vector* ptr = (Vector*)pthread_getspecific(errKey);
	if(ptr == NULL) {
		log_write(LEVEL_FATAL, "No error to eat");
		exit(1);
	}
	freeElems(ptr);
	vector_clear(ptr);
	pthread_setspecific(errKey, NULL);
}

void error_print() {
	Vector* ptr = (Vector*)pthread_getspecific(errKey);
	if(ptr == NULL) {
		log_write(LEVEL_FATAL, "No error to print");
		exit(1);
	}
	int index;
	struct Error* err = vector_getFirst(ptr, &index);
	while(err != NULL) {
		for(int i = 0; i < index; i++)
			fprintf(stderr, "%s", "   ");
		if(index >= 1)
			fprintf(stderr, "%s", "-> ");
		fprintf(stderr, "%s\n", err->message);
		err = vector_getNext(ptr, &index);
	}
	error_eat();
}

void error_abort() {
	error_print();
	exit(1);
}
