#include "ftp_command.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define ARGV_LEN 512

const char *ftp_command[] = {"ls", "cd", "get", "put", "help", "version", "pwd", "quit", NULL};
const int ftp_command_argc[] = {0, 1, 2, 2, 0, 0, 0, 0};

int resolv_ftp_command(const char *command_str, char **pArgv[]) {
	char buf[16];
	FTP_COMMAND_TAG tag;
	int i;
	sscanf(command_str, "%s", buf);
	for (tag = 0; ftp_command[tag] != NULL; tag++)
		if (strncmp(ftp_command[tag], buf, strlen(ftp_command[tag])) == 0)
			break;
	if (ftp_command[tag] == NULL)
		return -1;
	else {
		//指针后移准备读参数
		command_str += strlen(ftp_command[tag])+1;

		int argc = ftp_command_argc[tag];
		*pArgv = malloc((argc+1) * sizeof(char *));
		for (i = 0; i < argc; i++) {
			(*pArgv)[i] = malloc(ARGV_LEN * sizeof(char));
			sscanf(command_str, "%s", (*pArgv)[i]);
			if ((*pArgv)[i][0] == '\0')
				return -1;
			command_str += strlen((*pArgv)[i])+1;
		}
		(*pArgv)[argc] = NULL;
		return tag;
	}
}

void ftp_usage_str() {
	printf("Supported Command:\n");
	int i;
	for (i = 0; ftp_command[i] != NULL; i++)
		printf("%s\t", ftp_command[i]);
	printf("\n");
	printf("Usage:\n");
	printf("	ls #list the remote current working directory\n");
	printf("	cd PATH #change the remote current working directory to PATH\n");
	printf("	get REPATH LOPATH #download the remote REPATH to local LOPATH\n");
	printf("	put LOPATH REPATH #upload the local LOPATH to remote REPATH\n");
	printf("	help #get help information\n");
	printf("	version #get version information\n");
	printf("	pwd #get the remote current working directroy\n");
	printf("	quit #close the connection and exit\n");
	printf("\n");
}

void ftp_version_str() {
	printf("myftp 0.0.1\n");
	printf("WARNNING: This program is pre-alpha, so be careful to use it.\n");
	printf("License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>.\n");
	printf("This is free software: you are free to change and redistribute it.\n");
	printf("\nWritten by Given_92(huiyiqun@gmail.com)\n");
	printf("\n");
}
