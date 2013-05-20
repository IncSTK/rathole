#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <pwd.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>

extern char *decrypt_string(char *, char *);
extern char *encrypt_string(char *, char *, char *);

int main (int argc, char *argv[])
{
	int sock;
	char enc[769], *dec, bufkey[1024], bufsock[13], *pass;
	socklen_t slen;
	struct sockaddr_in saddr;
	fd_set fdset;
	slen = sizeof(saddr);

	if (argc < 3) {
		printf("Usage: rat <ip> <port>\n\n");
		exit(0);
	}
	memset(&saddr, 0, slen);
	saddr.sin_family = AF_INET;
	if ((inet_pton(AF_INET, argv[1], &saddr.sin_addr)) <= 0) {
		perror("inet_pton");
		exit(-1);
	}
	saddr.sin_port = htons(atoi(argv[2]));

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		exit(-1);
	}
	if (connect(sock, (struct sockaddr *)&saddr, slen) < 0) {
		perror("connect");
		exit(-1);
	}
	
	pass = getpass("\nPassword: ");

	FD_ZERO(&fdset);
	while (1) {
		FD_SET(0, &fdset);
		FD_SET(sock, &fdset);
		select(sock + 1, &fdset, NULL, NULL, NULL);
		if (FD_ISSET(0, &fdset)) {
			memset(bufkey, 0, sizeof(bufkey));
			memset(enc, 0, sizeof(enc));
			read(0, bufkey, sizeof(bufkey)-1);
			encrypt_string(pass, bufkey, enc);
			write(sock, enc, strlen(enc));
		}
		if (FD_ISSET(sock, &fdset)) {
			memset(bufsock, 0, sizeof(bufsock));
			recv(sock, bufsock, sizeof(bufsock)-1, MSG_WAITALL);
			dec = decrypt_string(pass, bufsock);
			write(1, dec, strlen(dec));
		}
	}
	shutdown(sock, 2);
	exit(0);
}
