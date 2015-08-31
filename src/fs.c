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
void getFiles(jmp_buf jmpBuf, const char* path, Vector* nameList)
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
		vector_init(jmpBuf, &name, sizeof(char), 100);
		vector_putListBack(jmpBuf, &name, path, strlen(path));
		vector_putBack(jmpBuf, &name, &slash);
		vector_putListBack(jmpBuf, &name, ent->d_name, strlen(ent->d_name)+1);

		vector_putBack(jmpBuf, nameList, &name.data);
		//Name is not destroyed because we want to keep the buffer around
	}	
	closedir(dir);
	vector_qsort(nameList, fileSort);
}
