#define _XOPEN_SOURCE
#include <shadow.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

#include "auth.h"

//0 for fail
//1 for success
int auth(char *login, char *key) {
	struct spwd *sp = getspnam(login);
	if (sp == NULL || sp->sp_pwdp == NULL) {
		fprintf(stderr, "Wrong user!\n");
		return 0;
	}
	char* shadow = sp->sp_pwdp;

	int rv = strcmp(shadow, crypt(key, shadow));
	if (rv == 0)
		return 1;
	else
		return 0;
}
