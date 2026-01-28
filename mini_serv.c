#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <stdio.h>
#include <stdlib.h>

char*	ARG = "Wrong number of arguments\n";
char*	FATAL = "Fatal error \n";
char ANNOUNCE[600000], MSG[600000], BUFF[1024][600000];
int sockfd, connfd, nfds, id, ids[1024], idx[1024] = {0};
struct sockaddr_in servaddr, cli; 
fd_set fds, rfds;
socklen_t len;


void err(char *str) {
	write(2, str, strlen(str));
	exit(1);
}

void announcement(char *str, int cli) {
	for (int fd = 0; fd < nfds; fd++)
		if (fd != sockfd && fd != cli && FD_ISSET(fd, &fds))
			send(fd, str, strlen(str), 0);
}

void join_client(int fd) {
	if (fd >= nfds)
		nfds = fd + 1;
	ids[fd] = id++;
	bzero(ANNOUNCE, sizeof(ANNOUNCE));
	sprintf(ANNOUNCE, "server: client %d just arrived\n", ids[fd]);
	announcement(ANNOUNCE, fd);
	FD_SET(fd, &fds);
}

void left_client(int fd) {
	FD_CLR(fd, &fds);
	bzero(ANNOUNCE, sizeof(ANNOUNCE));
	sprintf(ANNOUNCE, "server: client %d just left\n", ids[fd]);
	announcement(ANNOUNCE, fd);
	close(fd);
}

void share_msg(int fd){
	bzero(ANNOUNCE, sizeof(ANNOUNCE));
	for (int i = 0; MSG[i]; i++) {
		BUFF[ids[fd]][idx[ids[fd]]] = MSG[i];
		idx[ids[fd]]++;
		if (MSG[i] == '\n') {
			sprintf(ANNOUNCE, "client %d: %s", ids[fd], BUFF[ids[fd]]);
			announcement(ANNOUNCE, fd);
			bzero(BUFF[ids[fd]], idx[ids[fd]]);
			idx[ids[fd]] = 0;
			bzero(ANNOUNCE, sizeof(ANNOUNCE));
		}
	}
	bzero(MSG, sizeof(MSG));
}

void	mini_serv() {
	FD_ZERO(&fds);
	FD_SET(sockfd, &fds);
	nfds = sockfd + 1;

	while (1) {
		rfds = fds;
		if (select(nfds, &rfds, NULL, NULL, NULL) == -1)
			err(FATAL);
		for (int fd = 0; fd < nfds; fd++) {
			if (!FD_ISSET(fd, &rfds))
				continue;
			if (fd == sockfd) {
				len = sizeof(cli);
				connfd = accept(sockfd, (struct sockaddr *)&cli, &len);
				if (connfd > 0) { 
					join_client(connfd);
					break;
				}
			}
			else {
				if (recv(fd, MSG, 199999, 0) <= 0)
				{
					left_client(fd);
					break;
				}
				else
					share_msg(fd);
			}
		}
	}
}

int main(int ac, char **av) {

	if (ac != 2)
		err(ARG);
	// socket create and verification 
	sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (sockfd == -1) 
		err(FATAL);
	bzero(&servaddr, sizeof(servaddr)); 

	// assign IP, PORT 
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	servaddr.sin_port = htons(atoi(av[1])); 
  
	// Binding newly created socket to given IP and verification 
	if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) == -1)
		err(FATAL);
	if (listen(sockfd, 10) == -1)
		err(FATAL);

	
	mini_serv();
}