#include <config.h>
#include <iniparser.h>
#include <stdio.h>
#include <argp.h>
#include <stdbool.h>
#include <dirent.h>

#include "logger.h"
#include "vector.h"

const char* argp_program_version = PACKAGE_STRING;
const char* argp_program_bug_address = PACKAGE_BUGREPORT;

static char doc[] = "bard";
static char args_doc[] = "";
static struct argp_option options[] = {
	{ "config", 'c', "config", 0, "Configuration directory."},
	{ 0 }
};

struct arguments
{
	char* configDir;
};

static error_t parse_opt(int key, char* arg, struct argp_state *state)
{
	struct arguments *args = state->input;

	switch(key)
	{
		case 'c':
			args->configDir = arg;
			break;
		default:
			return ARGP_ERR_UNKNOWN;
	}
	return 0;
}

static struct argp argp = {options, parse_opt, args_doc, doc, 0, 0, 0};

int fileSort(const void* e1, const void* e2)
{
	return strcmp((char*)e1, (char*)e2);	
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
		Vector name;
		vector_init(&name, sizeof(char), 100);
		vector_putListBack(&name, path, strlen(path));
		vector_putBack(&name, &slash);
		vector_putListBack(&name, ent->d_name, strlen(ent->d_name)+1);

		vector_putBack(nameList, name.data);
		//Name is not destroyed because we want to keep the buffer around
	}	
	vector_qsort(nameList, fileSort);
}

int main(int argc, char **argv)
{
	struct arguments arguments = {0};
	argp_parse(&argp, argc, argv, 0, 0, &arguments);
	if(arguments.configDir == NULL)
	{
		log_write(LEVEL_ERROR, "Config directory required");
		exit(0);
	}
	log_write(LEVEL_INFO, arguments.configDir);

	Vector names;
	vector_init(&names, sizeof(Vector), 5);
	getFiles(arguments.configDir, &names);

	for(size_t i = 0; i < vector_size(&names); i++)
	{
		dictionary *conf = iniparser_load((char*)vector_get(&names, i));
		printf("%s\n", iniparser_getstring(conf, "unit:name", "NOT FOUND"));
		printf("%s\n", ((char*)vector_get(&names, i)));
	}

	return 0;
}
