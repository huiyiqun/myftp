#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <arpa/inet.h>
#include <pwd.h>
#include <linux/limits.h>
#include <fcntl.h>
#include <time.h>

#include "socket.h"
#include "str.h"
#include "close_fd.h"
#include "tag.h"
#include "auth.h"
#include "list.h"
#include "port.h"
#include "tag.h"

#define BACKLOG 10

#define DATABUFFERSIZE 102400
#define BUFFERSIZE 512
const char* PID_PATH = "/tmp/myftpd.pid";
const char* OUT_PATH = "/tmp/myftpd.out";
const char* ERR_PATH = "/tmp/myftpd.err";

void exit_proc(int sign) {
	unlink(PID_PATH);
	exit(sign);
}

int build_connect_ftpd(int type, struct sockaddr *addr) {
	int transform_fd;
	socklen_t sin_size = sizeof(struct sockaddr_in);

	if ((transform_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("Failed to create transform socket\n");
		return -1;
	}
	if (connect(transform_fd, addr, sin_size) == -1) {
		perror("Failed to connect transfrom socket\n");
		return -1;
	}
	return transform_fd;
}

char* read_argv(char *info, int request_len) {
	char *argv = (char *)malloc(sizeof(char)*strlen(info));
	int i;
	info += request_len + 1;
	for (i = 0; info[i] != '\r' && info[i] != '\n'; i++)
		argv[i] = info[i];
	argv[i] = '\0';
	return argv;
}


int stop_ftpd() {
	FILE *pid_file;
	pid_t pid;
	if ((pid_file = fopen(PID_PATH, "r")) == NULL) {
		perror("Can't open the pid file");
		return -1;
	}
	if (fscanf(pid_file, "%d", &pid) == EOF) {
		perror("Error occurs when reading pid");
		return -1;
	}
	fclose(pid_file);
	if (pid <= 0) {
		fprintf(stderr, "Error pid\n");
		return -1;
	}
	kill(pid, SIGTERM);
	if (unlink(PID_PATH) == -1) {
		perror("Can't delete the pid file");
		return -1;
	}
	return 0;
}

//0:正在运行
//1:pid文件找不到
int ftpd_status() {
	//测试pid文件是否存在
	struct stat buf;
	if (stat(PID_PATH, &buf) != -1 || errno != ENOENT) {
		return 0;
	}
	return 1;
}


int main(int argc, char* argv[]) {
	COMMAND_TAG command;
	if (argc > 1) {
		if (strcmp(argv[1], "test") == 0)
			command = TEST;
		else if (strcmp(argv[1], "start") == 0)
			command = START;
		else if (strcmp(argv[1], "stop") == 0)
			command = STOP;
		else if (strcmp(argv[1], "status") == 0) {
			printf((ftpd_status() == 0) ? "Myftpd is running\n" : "The pid file is unreachable\n");
			return 0;
		}
		else if (strcmp(argv[1], "help") == 0) {
			ftpd_help_str(argv[0]);
			return 0;
		}
		else if (strcmp(argv[1], "version") == 0) {
			ftpd_version_str();
			return 0;
		}
		else {
			ftpd_help_str(argv[0]);
			return -1;
		}
	}
	else {
		ftpd_help_str(argv[0]);
		return -1;
	}

	//判断是否以root权限运行
	if (geteuid() != 0) {
		fprintf(stderr, "Please run the program with root.\n");
		return -1;
	}

	if (command == STOP) {
		if (stop_ftpd())
			return 0;
		else
			return -1;
	}

	//判断TEST或START
	if (command == START) {

		//测试pid文件是否存在
		if (ftpd_status() == 0) {
			fprintf(stderr, "pid file exists. There may be a process running\n");
			return -1;
		}

		//后台运行
		int pid = fork();
		if (pid)
			exit(0);
		pid = setsid();
		pid = fork();
		if (pid) {
			FILE *pid_file;
			if ((pid_file = fopen(PID_PATH, "w")) == NULL) {
				perror("Can't open the pid file");
				return -1;
			}
			fprintf(pid_file, "%d", pid);
			fclose(pid_file);

			exit(0);
		}
		close_open_fds();
		umask(0);
		chdir("/");
		//至此,该进程成为一个守护进程
		
		
		//重定向标准输出和标准错误
		freopen(ERR_PATH, "w", stderr);
		freopen(OUT_PATH, "w", stdout);

		//取消缓冲区
		setbuf(stderr, NULL);
		setbuf(stdout, NULL);
	}
	else if (command != TEST) {//逻辑错误 
		fprintf(stderr, "Unknow Error\n");
		return -1;
	}


	//监视socket初始化
	int listen_fd;
	listen_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_fd == -1) {
		perror("Failed to create listen socket");
		exit_proc(-1);
	}

	struct sockaddr_in server_addr;

	//绑定监视socket地址
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(SERVPORT);
	bzero(&(server_addr.sin_zero), sizeof(server_addr.sin_zero));
	int rv = bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr));
	if (rv == -1) {
		perror("Failed to bind listen socket");
		exit_proc(-1);
	}

	//监听端口
	rv = listen(listen_fd, BACKLOG);
	if (rv == -1) {
		perror("Listen socket failed to listen");
		exit_proc(-1);
	}
	
	//接收信息
	while (1) {
		int service_fd;
		struct sockaddr_in client_addr;
		socklen_t addrlen = sizeof(struct sockaddr);
		service_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &addrlen);
		if (service_fd == -1)
			perror("Listen socket failed to accept connection");
		printf("received a request from %s\n", inet_ntoa(client_addr.sin_addr));
		if (!fork()) {//子进程
			char *msg;//将要发送的消息
			char info[BUFFERSIZE];//收到的消息

			char data[DATABUFFERSIZE];//传输数据的缓存
			
			msg = "220 Hello, Let's begin!\r\n";
			send(service_fd, msg, strlen(msg), 0);

			struct passwd *guest = NULL;
			int logined = 0;

			char *login = NULL;
			char *passwd = NULL;

			char *dir = NULL;

			int transform_fd;//数据传输socket

			//根据命令回应
			while (1) {
				bzero(&info, BUFFERSIZE*sizeof(char));
				int recv_info = recv(service_fd, info, BUFFERSIZE, 0);
				if (recv_info == -1) {
					perror("Service socket failed to recv info");
					continue;
				}
				else if (recv == 0) {
					fprintf(stderr, "Connection closed");
					break;
				}
				int request_tag = resolv_request(info);

				//若没有登录 拒绝服务
				if (!login && request_tag != USER && request_tag != QUIT && request_tag != PASS) {
					printf("%s", info);
					printf("refuse request!\n");
					msg = "530 Haven't login\r\n";
					send(service_fd, msg, strlen(msg), 0);
					continue;
				}

				socklen_t sin_size = sizeof(struct sockaddr_in);

				switch (request_tag) {
					case QUIT:
						printf("%s", info);
						msg = "221 See you later...\r\n";
						send(service_fd, msg, strlen(msg), 0);
						goto LOOP_OUT;
					case USER:
						printf("%s", info);
						login = read_argv(info, 4);
						msg = "331 Well, passwd please\r\n";
						send(service_fd, msg, strlen(msg), 0);
						break;
					case PASS:
						printf("%s", info);
						passwd = read_argv(info, 4);
						int flag;
						if (auth(login, passwd)) {
							msg = "230 Login successfully.\r\n";
							printf("%s", msg);
							guest = getpwnam(login);
							logined = 1;
							seteuid(guest->pw_uid);
							chdir(guest->pw_dir);
						}
						else {
							sleep(3);
							msg = "530 Fail to login, please check.\r\n";
							printf("%s", msg);
							if (login != NULL)
								free(login);
							login = NULL;
							if (passwd != NULL)
								free(passwd);
							passwd = NULL;
						}
						send(service_fd, msg, strlen(msg), 0);
						break;
					case CWD:
						printf("%s", info);
						dir = read_argv(info, 3);
						errno = 0;
						if (chdir(dir) == -1) {
							if (errno != 0) {
								msg = (char *)malloc(sizeof(char)*(PATH_MAX+BUFFERSIZE));
								sprintf(msg, "550 Cannot chdir to %s: %s.\r\n", dir, sys_errlist[errno]);
							}
							else {
								msg = (char *)malloc(sizeof(char)*(PATH_MAX+BUFFERSIZE));
								sprintf(msg, "550 Cannot chdir to %s.\r\n", dir);
							}
							send(service_fd, msg, strlen(msg), 0);
							free(msg);
						}
						else {
							msg = "250 OK.\r\n";
							send(service_fd, msg, strlen(msg), 0);
						}
						free(dir);
						break;
					case PWD:
						printf("%s", info);
						dir = (char *)malloc(sizeof(char)*(PATH_MAX));
						getcwd(dir, PATH_MAX);
						msg = (char *)malloc(sizeof(char)*(PATH_MAX+BUFFERSIZE));
						sprintf(msg, "257 \"%s \" is the current working directory\r\n", dir);
						send(service_fd, msg, strlen(msg), 0);
						free(dir);
						free(msg);
						break;
					case PORT:
						printf("%s", info);
						int port[2];
						int addr[4];
						sscanf(info, "%*s %d,%d,%d,%d,%d,%d", &addr[0], &addr[1], &addr[2], &addr[3], &port[0], &port[1]);
						client_addr.sin_port = htons(port[0] * 256 + port[1]);
						char addr_str[16];
						bzero(addr_str, 16*sizeof(char));
						sprintf(addr_str, "%d.%d.%d.%d", addr[0], addr[1], addr[2], addr[3]);
						inet_aton(addr_str, &client_addr.sin_addr);
						sin_size = sizeof(struct sockaddr_in);
						msg = (char *)malloc(sizeof(char)*BUFFERSIZE);
						sprintf(msg, "200 PORT %s:%d OK.\r\n", addr_str, ntohs(client_addr.sin_port));
						send(service_fd, msg, strlen(msg)*sizeof(char), 0);
						free(msg);
						break;
					case RETR:
						printf("%s", info);
						if (!fork()) {
							char *pathname = read_argv(info, 4);
							FILE *file = fopen(pathname, "r");
							if (file == NULL) {
								msg = (char *)malloc(sizeof(char)*(PATH_MAX+BUFFERSIZE));
								sprintf(msg, "550 Can't open %s: %s\r\n", pathname, sys_errlist[errno]);
								send(service_fd, msg, strlen(msg), 0);
								free(msg);
							}
							else if ((transform_fd = build_connect_ftpd(PORT, (struct sockaddr *)&client_addr)) == -1) {
								msg = "425 Fail to build connection\r\n";
								send(service_fd, msg, strlen(msg), 0);
							}
							else {
								msg = "150 Ready to transform\r\n";
								send(service_fd, msg, strlen(msg), 0);
								ssize_t data_size;
								time_t start = time(NULL);
								while (1) {
									data_size = fread(data, 1, DATABUFFERSIZE, file);
									send(transform_fd, data, data_size, 0);
									if (data_size < DATABUFFERSIZE)
										break;
								}
								fclose(file);
								time_t end = time(NULL);
								shutdown(transform_fd, SHUT_RDWR);
								if (close(transform_fd) == 0) {
									msg = (char *)malloc(sizeof(char)*(PATH_MAX+BUFFERSIZE));
									sprintf(msg, "226-File successfully transferred\r\n226 %ld seconds (measured here).\r\n", (end-start));
									send(service_fd, msg, strlen(msg), 0);
								}
								else {
									perror("Fail to close transform socket");
									msg = "451 Failed to close socket?";
									send(service_fd, msg, strlen(msg), 0);
								}
							}
							free(pathname);
						}
						break;
					case STOR:
						printf("%s", info);
						if (!fork()) {
							char *pathname = read_argv(info, 4);
							FILE *file = fopen(pathname, "w");
							if (file == NULL) {
								msg = (char *)malloc(sizeof(char)*(PATH_MAX+BUFFERSIZE));
								sprintf(msg, "550 Can't open %s: %s\r\n", pathname, sys_errlist[errno]);
								send(service_fd, msg, strlen(msg), 0);
								free(msg);
							}
							else if ((transform_fd = build_connect_ftpd(PORT, (struct sockaddr *)&client_addr)) == -1) {
								msg = "425 Fail to build connection\r\n";
								send(service_fd, msg, strlen(msg), 0);
							}
							else {
								msg = "150 Ready to transform\r\n";
								send(service_fd, msg, strlen(msg), 0);
								ssize_t data_size;
								time_t start = time(NULL);
								while (1) {
									data_size = recv(transform_fd, data, DATABUFFERSIZE, 0);
									fwrite(data, 1, data_size, file);
									if (data_size <= 0)
										break;
								}
								fclose(file);
								time_t end = time(NULL);
								shutdown(transform_fd, SHUT_RDWR);
								if (close(transform_fd) == 0) {
									msg = (char *)malloc(sizeof(char)*(PATH_MAX+BUFFERSIZE));
									sprintf(msg, "226-File successfully transferred\r\n226 %ld seconds (measured here).\r\n", (end-start));
									send(service_fd, msg, strlen(msg), 0);
								}
								else {
									perror("Fail to close transform socket");
									msg = "451 Failed to close socket?";
									send(service_fd, msg, strlen(msg), 0);
								}
							}
							free(pathname);
						}
						break;
					case LIST:
						printf("%s", info);
						if (!fork()) {
							if (list_cwd(data, DATABUFFERSIZE) != 0) {
								msg = "452 The data is too large to transform.\r\n";
								send(service_fd, msg, strlen(msg), 0);
							}
							else if ((transform_fd = build_connect_ftpd(PORT, (struct sockaddr *)&client_addr)) == -1) {
								msg = "425 Fail to build connection\r\n";
								send(service_fd, msg, strlen(msg), 0);
							}
							else {
								msg = "150 Ready to transform\r\n";
								send(service_fd, msg, strlen(msg), 0);
								send(transform_fd, data, strlen(data), 0);
								shutdown(transform_fd, SHUT_RDWR);
								if (close(transform_fd) == 0) {
									msg = "226-Options: -la\r\n226 Well done\r\n";
									send(service_fd, msg, strlen(msg), 0);
								}
								else {
									perror("Fail to close transform socket");
									msg = "451 Failed to close socket?";
									send(service_fd, msg, strlen(msg), 0);
								}
							}
							exit(0);
						}
						break;
					case SYST:
						printf("%s", info);
						msg = "215 UNIX Type: L8\r\n";
						send(service_fd, msg, strlen(msg), 0);
						break;
					case TYPE:
						printf("%s", info);
						msg = "200 Well done.\r\n";
						send(service_fd, msg, strlen(msg), 0);
						break;
					default:
						printf("Unknown Request:%s\n", info);
						msg = "551 Unknown Request.\r\n";
						send(service_fd, msg, strlen(msg)*sizeof(char), 0);
						break;
				}
			}
LOOP_OUT:
			printf("The service for %s is end.\n", inet_ntoa(client_addr.sin_addr));
			shutdown(service_fd, SHUT_RDWR);
			return 0;
		}
	}
}
