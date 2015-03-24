#include <config.h>
#include <iniparser.h>
#include <stdio.h>

#include "logger.h"

int main(int argc, char **argv)
{
	log_write(LEVEL_WARNING, "Hello world");
	return 1;
}
