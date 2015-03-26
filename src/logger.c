#include "logger.h"
#include <stdio.h>

static const char *const LevelNames[] = {
	"Info",
	"Warning",
	"Error",
};

static const char* LevelStr(const LOG_Level level)
{
	return LevelNames[level];
}

void log_write(LOG_Level level, const char* message)
{
	printf("[%s] %s\n", LevelStr(level), message);
}
