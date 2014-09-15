CC = clang
CFLAGS = -g

all: server client

server: ftpd
client: ftp

ftpd: close_fd.o ftpd.o str.o auth.o list.o tag.o str.o tag.o socket.o -lcrypt
ftp: ftp.o ftp_command.o socket.o -lreadline

close_fd.o: close_fd.c close_fd.h
str.o: str.c str.h ftp_command.h
auth.o: auth.c auth.h
socket.o: socket.c socket.h
list.o: list.c list.h
tag.o: tag.c tag.h
ftpd.o: ftpd.c str.h close_fd.h tag.h auth.h list.h port.h
ftp.o: ftp.c port.h ftp_command.h
ftp_command.o: ftp_command.c ftp_command.h

clean:
	rm -f ftpd ftp close_fd.o ftpd.o str.o auth.o list.o ftp.o tag.o ftp_command.o socket.o
