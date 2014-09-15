#ifndef _TAG_H
#define _TAG_H
typedef enum {START, STOP, TEST} COMMAND_TAG;
typedef enum {QUIT, USER, CWD, PWD, PORT, RETR, LIST, PASS, SYST, TYPE, STOR} REQUEST_TAG;

extern int resolv_request(char *request);
#endif
