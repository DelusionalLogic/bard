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
#ifndef FS_H
#define FS_H

#include <dirent.h>
#include <string.h>
#include "logger.h"
#include "vector.h"

char* pathAppend(const char* path, const char* path2);

int fileSort(const void* e1, const void* e2);

//nameList is a vector of string (char*)
void getFiles(const char* path, Vector* nameList);

#endif
