#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include "socket.h"

void get_sockinfo(int sock, char **addr, int *port) {
	struct sockaddr_in my_addr;
	socklen_t sockaddr_len = sizeof(struct sockaddr);

	int rv;
	rv = getsockname(sock, (struct sockaddr *)&my_addr, &sockaddr_len);
	if (rv == -1) {
		perror("getname");
		return;
	}
	*addr = inet_ntoa(my_addr.sin_addr);
	*port = ntohs(my_addr.sin_port);
}

int get_sockport(int sock) {
	char *addr;
	int port;
	get_sockinfo(sock, &addr, &port);
	return port;
}

char *get_sockaddr(int sock) {
	char *addr;
	int port;
	get_sockinfo(sock, &addr, &port);
	return addr;
}
