#include "logger.h"
#include <stdio.h>
#include <stdarg.h>

static const char *const LevelNames[] = {
	"Info",
	"Warning",
	"Error",
	"FATAL",
};

static const char* LevelStr(const LOG_Level level)
{
	return LevelNames[level];
}

void log_write(LOG_Level level, const char* format, ...)
{
	va_list args;
	fprintf(stderr, "[%s]", LevelStr(level));
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	fprintf(stderr, "\n");
}
