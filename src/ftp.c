#include <stdio.h>
#include <sys/socket.h>
#include <netdb.h>
#include <strings.h>
#include <arpa/inet.h>
#include <string.h>
#include <strings.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <unistd.h>
#include <stdlib.h>

#include "port.h"
#include "ftp_command.h"
#include "socket.h"

#define BUFFERSIZE 512
#define MSGSIZE 512
#define HOSTNAMESIZE 512
#define ADDRSIZE 32
#define DATASIZE 256

void exit_with_socket_closed(int socket, int sign) {
	if (socket != -1)
		close(socket);
	exit(sign);
}

int printf_info(int socket) {
	int first = 1;
	ssize_t info_size;
	int infoNo;
	char buf[BUFFERSIZE+1];
	while (1) {
		info_size = recv(socket, buf, BUFFERSIZE, 0);
		buf[info_size] = '\0';
		if (first) {
			first = 0;
			sscanf(buf, "%d", &infoNo);
		}
		printf("%s", buf);
		if (info_size != BUFFERSIZE)
			break;
	}
	if (info_size == 0)
		return 0;
	else if (info_size == -1)
		return -1;
	else
		return infoNo;
}

int get_port(int *portNo, int *sock_fd) {
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == -1) {
		perror("build socket");
		return -1;
	}

	int rv;
	struct sockaddr_in my_addr;
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = 0;
	my_addr.sin_addr.s_addr = INADDR_ANY;
	bzero(&(my_addr.sin_zero), 8);

	socklen_t sockaddr_len = sizeof(struct sockaddr);

	rv = bind(sock, (struct sockaddr *)&my_addr, sizeof(struct sockaddr));
	if (rv == -1) {
		perror("build socket");
		return -1;
	}

	rv = listen(sock, 10);
	if (rv == -1) {
		perror("listen");
		return -1;
	}

	rv = getsockname(sock, (struct sockaddr *)&my_addr, &sockaddr_len);
	if (rv == -1) {
		perror("getname");
		return -1;
	}

	*portNo = ntohs(my_addr.sin_port);
	*sock_fd = sock;
	return 0;
}

int recv_and_write(int sock_fd, FILE *file) {
	int transform_fd;
	struct sockaddr_in addr;
	socklen_t addrlen = sizeof(struct sockaddr);
	transform_fd = accept(sock_fd, (struct sockaddr *)&addr, &addrlen);
	if (transform_fd == -1) {
		perror("Failed to accept");
		return -1;
	}
	char buf[DATASIZE];
	int rv;
	while (1) {
		rv = recv(transform_fd, buf, DATASIZE, 0);
		if (rv < 0) {
			perror("Failed to recieve");
			close(transform_fd);
			return -1;
		}
		fwrite(buf, 1, rv, file);
		if (rv < DATASIZE)
			break;
	}
	close(transform_fd);
	return 0;
}

int read_and_send(int sock_fd, FILE *file) {
	int transform_fd;
	struct sockaddr_in addr;
	socklen_t addrlen = sizeof(struct sockaddr);
	transform_fd = accept(sock_fd, (struct sockaddr *)&addr, &addrlen);
	if (transform_fd == -1) {
		perror("Failed to accept");
		return -1;
	}
	char buf[DATASIZE];
	int rv;
	while (1) {
		rv = fread(buf, 1, DATASIZE, file);
		if (rv < 0) {
			perror("Failed to read");
			close(transform_fd);
			return -1;
		}
		send(transform_fd, buf, rv, 0);
		if (rv < DATASIZE)
			break;
	}
	close(transform_fd);
	return 0;
}

int build_connect_ftp(int control_fd) {
	int portNo;
	int transform_fd;
	char msg[MSGSIZE];
	if (get_port(&portNo, &transform_fd) == -1) {
		perror("Failed to port");
		return -1;
	}
	else {
//		printf("%d\n", portNo);
		int addr[4];
		int port[2];
		port[0] = portNo / 256;
		port[1] = portNo % 256;
		char *addr_str = get_sockaddr(control_fd);
		sscanf(addr_str, "%d.%d.%d.%d", &addr[0], &addr[1], &addr[2], &addr[3]);
		bzero(msg, sizeof(msg));
		sprintf(msg, "PORT %d,%d,%d,%d,%d,%d\r\n", addr[0], addr[1], addr[2], addr[3], port[0], port[1]);
		send(control_fd, msg, strlen(msg), 0);
		int infoNo = printf_info(control_fd);
		if (infoNo != 200) {
			fprintf(stderr, "Unkowned response number\n");
			return -1;
		}
	}
	return transform_fd;
}

int type_I(int socket) {
	int infoNo;
	char msg[MSGSIZE];
	sprintf(msg, "TYPE I\r\n");
	send(socket, msg, strlen(msg), 0);
	infoNo = printf_info(socket);
	if (infoNo != 200)
		return -1;
	else
		return 0;
}


const char* prompt = "ftp> ";


int main(int argc, char *argv[]) {

	if (argc != 2) {
		fprintf(stderr, "Usage: %s HOST\n", argv[0]);
		return -1;
	}

	struct hostent *server;
	server = gethostbyname(argv[1]);
	if (server == NULL) {
		fprintf(stderr, "Cannot resolv the host name.\n");
		return -1;
	}
	else if (server->h_addrtype != AF_INET) {
		fprintf(stderr, "Unsupported network, only IPv4 is supported now.\n");
		return -1;
	}

	int i;
	char str[32];
	int control_fd = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(SERVPORT);
	for (i = 0; server->h_addr_list[i] != NULL; i++) {
		int j;
		for (j = 0; j < server->h_length; j++) {
			((char *)&(server_addr.sin_addr.s_addr))[j] = server->h_addr_list[i][j];
		}
		printf("Try the address: %s\n", inet_ntoa(server_addr.sin_addr));
		if (connect(control_fd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr_in)) == -1)
			perror("	fail");
		else
			break;
	}
	if (server->h_addr_list[i] == NULL) {
		fprintf(stderr, "Cannot connect\n");
		return -1;
	}
	else
		printf("Connected!\n");

	int infoNo;
	char msg[MSGSIZE];
	char *pChar;

	//Login name
	infoNo = printf_info(control_fd);
	if (infoNo != 220) {
		fprintf(stderr, "Unkowned response number\n");
		exit_with_socket_closed(control_fd, -1);
	}
	bzero(msg, MSGSIZE);
	pChar = readline("Login name: ");
	sprintf(msg, "USER %s\r\n", pChar);
	send(control_fd, msg, strlen(msg), 0);

	//Password
	infoNo = printf_info(control_fd);
	if (infoNo != 331) {
		fprintf(stderr, "Unkowned response number\n");
		exit_with_socket_closed(control_fd, -1);
	}
	bzero(msg, MSGSIZE);
	pChar = getpass("Password: ");
	sprintf(msg, "PASS %s\r\n", pChar);
	send(control_fd, msg, strlen(msg), 0);

	//Checkin result
	infoNo = printf_info(control_fd);
	if (infoNo != 230) {
		fprintf(stderr, "Failed to check in.\n");
		exit_with_socket_closed(control_fd, -1);
	}
	else {
		bzero(msg, MSGSIZE);
		strcpy(msg, "SYST\r\n");
		send(control_fd, msg, strlen(msg), 0);
		printf_info(control_fd);
	}

	while(1) {
		char **argv_ftp;
		pChar = readline(prompt);
		int tag;
		if (pChar == NULL) {
			tag = QUIT;
		}
		else
			tag = resolv_ftp_command(pChar, &argv_ftp);
		if (tag == -1) {
			fprintf(stderr, "Wrong command!\n");
			ftp_usage_str();
			continue;
		}
		else {
			int transform_fd;
			FILE *file;
			switch (tag) {
				case LS:
					if (type_I(control_fd) == -1)
						fprintf(stderr, "Transform may fail!");
					transform_fd = build_connect_ftp(control_fd);
					sprintf(msg, "LIST\r\n");
					send(control_fd, msg, strlen(msg), 0);
					infoNo = printf_info(control_fd);
					if (infoNo != 150) {
						fprintf(stderr, "Unkowned response number\n");
						break;
					}
					recv_and_write(transform_fd, stdout);
					close(transform_fd);
					infoNo = printf_info(control_fd);
					if (infoNo != 226) {
						fprintf(stderr, "Unkowned response number\n");
						break;
					}
					break;
				case CD:
					sprintf(msg, "CWD %s\r\n", argv_ftp[0]);
					send(control_fd, msg, strlen(msg), 0);
					infoNo = printf_info(control_fd);
					break;
				case GET:
					file = fopen(argv_ftp[1], "w");
					if (file == NULL) {
						perror("Fail to open the file to write");
						break;
					}
					if (type_I(control_fd) == -1)
						fprintf(stderr, "Transform may fail!");
					transform_fd = build_connect_ftp(control_fd);
					sprintf(msg, "RETR %s\r\n", argv_ftp[0]);
					send(control_fd, msg, strlen(msg), 0);
					infoNo = printf_info(control_fd);
					if (infoNo != 150) {
						fprintf(stderr, "Unkowned response number\n");
						break;
					}
					recv_and_write(transform_fd, file);
					close(transform_fd);
					infoNo = printf_info(control_fd);
					if (infoNo != 226) {
						fprintf(stderr, "Unkowned response number\n");
						break;
					}
					break;
				case PUT:
					file = fopen(argv_ftp[0], "r");
					if (file == NULL) {
						perror("Fail to open the file to read");
						break;
					}
					if (type_I(control_fd) == -1)
						fprintf(stderr, "Transform may fail!");
					transform_fd = build_connect_ftp(control_fd);
					sprintf(msg, "STOR %s\r\n", argv_ftp[1]);
					send(control_fd, msg, strlen(msg), 0);
					infoNo = printf_info(control_fd);
					if (infoNo != 150) {
						fprintf(stderr, "Unkowned response number\n");
						break;
					}
					read_and_send(transform_fd, file);
					close(transform_fd);
					infoNo = printf_info(control_fd);
					if (infoNo != 226) {
						fprintf(stderr, "Unkowned response number\n");
						break;
					}
					break;
				case HELP:
					ftp_usage_str();
					break;
				case VERSION:
					ftp_version_str();
					break;
				case PWD:
					sprintf(msg, "PWD\r\n");
					send(control_fd, msg, strlen(msg), 0);
					infoNo = printf_info(control_fd);
					break;
				case QUIT:
					sprintf(msg, "QUIT\r\n");
					send(control_fd, msg, strlen(msg), 0);
					infoNo = printf_info(control_fd);
					goto LOOP_OUT;
				default:
					break;
			}
		}



//		sprintf(msg, "%s\r\n", pChar);
//		send(control_fd, msg, strlen(msg), 0);
//		infoNo = printf_info(control_fd);
	}
LOOP_OUT:
	
	exit_with_socket_closed(control_fd, 0);
}
