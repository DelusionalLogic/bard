#ifndef LOGGER_H
#define LOGGER_H

typedef enum {
	LEVEL_WARNING
} LOG_Level;

void log_write(LOG_Level level, char* message);

#endif
