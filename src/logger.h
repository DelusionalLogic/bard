#ifndef LOGGER_H
#define LOGGER_H

typedef enum {
	LEVEL_INFO,
	LEVEL_WARNING,
	LEVEL_ERROR,
} LOG_Level;

void log_write(LOG_Level level, const char* format, ...);

#endif
