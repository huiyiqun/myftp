#ifndef _SOCKET_H
#define _SOCKET_H

extern void get_sockinfo(int sock, char **addr, int *port);
extern int get_sockport(int sock);
extern char *get_sockaddr(int sock);

#endif
