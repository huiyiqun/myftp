#include <string.h>
#include "tag.h"


int resolv_request(char *request) {
	if (strncmp("QUIT", request, 4) == 0)
		return QUIT;
	else if (strncmp("USER", request, 4) == 0)
		return USER;
	else if (strncmp("CWD", request, 3) == 0)
		return CWD;
	else if (strncmp("PWD", request, 3) == 0)
		return PWD;
	else if (strncmp("PORT", request, 4) == 0)
		return PORT;
	else if (strncmp("RETR", request, 4) == 0)
		return RETR;
	else if (strncmp("LIST", request, 4) == 0)
		return LIST;
	else if (strncmp("PASS", request, 4) == 0)
		return PASS;
	else if (strncmp("SYST", request, 4) == 0)
		return SYST;
	else if (strncmp("TYPE", request, 4) == 0)
		return TYPE;
	else if (strncmp("STOR", request, 4) == 0)
		return STOR;
	else
		return -1;
}
