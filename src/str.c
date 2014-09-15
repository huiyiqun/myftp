#include "str.h"
#include <stdio.h>
#include "ftp_command.h"

void ftpd_help_str(char const *argv_0) {
	printf("Usage: %s COMMAND\n", argv_0);
	printf("	test	run the program frontground\n");
	printf("	start	run the program background\n");
	printf("	stop	kill the program background\n");
	printf("	help	output the help message\n");
	printf("	version	output the version message\n");
}

void ftpd_version_str() {
	printf("myftpd 0.0.1\n");
	printf("WARNNING: This program is pre-alpha, so be careful to use it.\n");
	printf("License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>.\n");
	printf("This is free software: you are free to change and redistribute it.\n");
	printf("\nWritten by Given_92(huiyiqun@gmail.com)\n");
}

