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
#include "fs.h"
#include <stdio.h>

char* pathAppend(const char* path, const char* path2) {
	size_t pathLen = strlen(path);
	size_t additionalSpace = 0;
	if(path[pathLen-1] != '/')
		additionalSpace = 1;
	char* newPath = malloc(pathLen + strlen(path2) + additionalSpace + 1);
	strcpy(newPath, path);
	if(additionalSpace)
		newPath[pathLen] = '/';
	strcpy(newPath + pathLen + additionalSpace, path2);
	return newPath;
}

int fileSort(const void* e1, const void* e2)
{
	return strcmp(*(char**)e1, *(char**)e2);	
}

//nameList is a vector of string (char*)
void getFiles(const char* path, Vector* nameList)
{
	DIR* dir;
	struct dirent *ent;
	char slash = '/';

	if((dir = opendir(path)) == NULL)
	{
		log_write(LEVEL_ERROR, "Couldn't open directory");
		exit(1);
	}
	while((ent = readdir(dir)) != NULL)
	{
		if(ent->d_name[0] == '.') 
			continue;
		if(ent->d_type != DT_REG && ent->d_type != DT_LNK)
			continue;
		Vector name;
		vector_init(&name, sizeof(char), 100);
		vector_putListBack(&name, path, strlen(path));
		vector_putBack(&name, &slash);
		vector_putListBack(&name, ent->d_name, strlen(ent->d_name)+1);

		vector_putBack(nameList, &name.data);
		//Name is not destroyed because we want to keep the buffer around
	}	
	closedir(dir);
	vector_qsort(nameList, fileSort);
}
