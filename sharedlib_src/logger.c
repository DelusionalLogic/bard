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
#include "logger.h"
#include <stdio.h>
#include <stdarg.h>

static const char *const LevelNames[] = {
	"Info",
	"Warning",
	"Error",
	"FATAL",
};

static const char* LevelStr(const Log_Level level)
{
	return LevelNames[level];
}

void log_write(Log_Level level, const char* format, ...)
{
	va_list args;
	fprintf(stderr, "[%s]", LevelStr(level));
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	fprintf(stderr, "\n");
}
