#include <stddef.h>

#ifndef _FTP_COMMAND_H
#define _FTP_COMMAND_H
extern const char *ftp_command[];
extern const int ftp_command_argc[];

typedef enum {LS, CD, GET, PUT, HELP, VERSION, PWD, QUIT} FTP_COMMAND_TAG;

extern int resolv_ftp_command(const char *command_str, char **argv[]);
extern void ftp_version_str();
extern void ftp_usage_str();
#endif
