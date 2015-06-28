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
