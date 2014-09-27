#ifndef CONFIG_H
#define CONFIG_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

struct configItem
{
	char key[20];
	char value[50];
};

void config(char *configFilePath, struct configItem * configVar, int configNum);

#endif
