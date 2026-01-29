#include <errno.h> // COPIÉ DU MAIN DEBUT
#include <string.h> //
#include <unistd.h> //
#include <netdb.h> //
#include <sys/socket.h> //
#include <netinet/in.h> // FIN

#include <sys/select.h> // trois ajouts
#include <stdio.h>
#include <stdlib.h>

char*	ARG = "Wrong number of arguments\n";
char*	FATAL = "Fatal error \n";

char ANNOUNCE[600020], // un buffer pour mettre les annonces 600020 important pour -Werror
	MSG[600000], // un buffer pour stocker ce que recv recoit
	BUFF[1024][600000]; // un buffer de 600000 par client

int sockfd, // COPIE DU MAIN, fd du serv
	connfd, // COPIE DU MAIN, fd du nouveau client
	nfds, // nombre de fd total
	id, // id (en cours) a donner au nouveau client
	ids[1024], // liste des ids de tous les clients
	idx[1024] = {0}; // un index par client pour parser MSG

fd_set fds, // liste (fd set) necessaire pour select
	rfds;	// copie de la liste pour donner a select

struct sockaddr_in servaddr, cli; // COPIÉ DU MAIN (Passer len en socklen_t)
socklen_t len; // len du main passé en socklen_t


void err(char *str) { // fonction erreur simple
	write(2, str, strlen(str));
	exit(1);
}

void announcement(char *str, int cli) {
	for (int fd = 0; fd < nfds; fd++)
	// parcourir tous les fds

		if (fd != sockfd 
			// si le fd different de sockfd(fd sur serv)
			&& fd != cli
			// et different du client emetteur
			&& FD_ISSET(fd, &fds))	
			// et fd est set dans fds(fd set)

			send(fd, str, strlen(str), 0);
			// send l'annonce a fd (repeat)
}

void join_client(int fd) {
	if (fd >= nfds)
		nfds = fd + 1;
		// nfds = le plus grand fd de la liste donc si fd > nfds on l'update
	ids[fd] = id++;
	// on donne au nouveau client l'id+1 par rapport au dernier

	bzero(ANNOUNCE, sizeof(ANNOUNCE)); // effacer announce
	sprintf(ANNOUNCE, "server: client %d just arrived\n", ids[fd]); // remplir announce avec ce quon veut, en l'occurence copier coller ce qui est dit dans le sujet
	announcement(ANNOUNCE, fd); //envoyer à tout le monde
	// trois lignes dont se souvenir, se répètent 2 autres fois

	FD_SET(fd, &fds);
	// ajouter le nouveau client dans la liste fds (fd set)
}

void left_client(int fd) {
	FD_CLR(fd, &fds);
	// enlever client de la liste fds

	bzero(ANNOUNCE, sizeof(ANNOUNCE));
	sprintf(ANNOUNCE, "server: client %d just left\n", ids[fd]); // remplir avec ce qui est dit dans le sujet pour la deco d'un client
	announcement(ANNOUNCE, fd);
	// trois memes lignes sauf que cest left

	close(fd);
	// et on close le fd
}

void share_msg(int fd){
	for (int i = 0; MSG[i]; i++) {
		// parcourt msg

		BUFF[ids[fd]][idx[ids[fd]]] = MSG[i];
		// ecrit MSG[i] dans le buffer du client (ids[fd]) à l'index idx[ du client ids[fd]]

		idx[ids[fd]]++; 
		// incrementer l'index pour qu'il suive i

		if (MSG[i] == '\n') {
			// quand on trouve un '\n' alors il faut envoyer

			bzero(ANNOUNCE, sizeof(ANNOUNCE));
			sprintf(ANNOUNCE, "client %d: %s", ids[fd], BUFF[ids[fd]]); // remplir avec ce qui est dit dans le sujet
			announcement(ANNOUNCE, fd);
			// les trois memes lignes avec variation

			bzero(BUFF[ids[fd]], idx[ids[fd]]); // attention ce bzero est pas comme les autres, il se fait jusqu'a l'index
			idx[ids[fd]] = 0;
			// reset du buffer du client (ids[fd]) ainsi que de son index
		}
	}
	bzero(MSG, sizeof(MSG));
	// reset MSG apres l'avoir parsé
}

void	mini_serv() {
	// initialisation
	FD_ZERO(&fds);
	// initialiser la liste fds avec fd_zero
	FD_SET(sockfd, &fds);
	// ajouter sockfd (le fd du serv) dans la liste
	nfds = sockfd + 1;
	// initialiser le nombre de fd nfds à celui du serv + 1

	while (1) { // boucle infinie du serveur

		rfds = fds; // copier fds dans un temp (rfds) car select le modifie
		if (select(nfds, &rfds, NULL, NULL, NULL) == -1)
			err(FATAL); // si select echoue, fatal

		for (int fd = 0; fd < nfds; fd++) {
		// parcourir tous les fd, meme ceux deconnectés/qui existent pas

			if (!FD_ISSET(fd, &rfds)) // si deconnectés/existe pas on le saute
				continue;

			if (fd == sockfd) { // condition qui determine si le fd est un nouveau client

				len = sizeof(cli); // DEBUT COPIE MAIN
				connfd = accept(sockfd, (struct sockaddr *)&cli, &len); //
				if (connfd > 0) { // FIN COPIE MAIN - inverser la condition (auparavant < 0, maintenant > 0)

					join_client(connfd); // on le join si accept reussie
					break; // dès modification des clients (ajout ou retrait) on break pour recommencer la boucle et repasser par select
				}
			}
			else { // si cest pas un nouveau client :
				if (recv(fd, MSG, 199999, 0) <= 0)
				// on ecoute ce quil envoie, si cest <= 0 cest quil est buggé ou deco
				{
					left_client(fd); // donc on le retire
					break; // ET ON BREAK car on a modifié la liste fds et donc faut repasser par select
				}
				else
				// et si recv a recu un message
					share_msg(fd); // on l'envoie
			}
		}
	}
}

int main(int ac, char **av) {

	if (ac != 2) {
		err(ARG);
	}

	// socket create and verification 										      	    			    		//  DEBUT COPIE MAIN
	sockfd = socket(AF_INET, SOCK_STREAM, 0);							      		    			    	//
	if (sockfd == -1)															              	    			    	  //
		err(FATAL);																                	    			    	  // changer les conditions du main pour seulement err fatal
	bzero(&servaddr, sizeof(servaddr));										      	    			    		//

	// assign IP, PORT														              	    			    	  //
	servaddr.sin_family = AF_INET; 												      	    			    	  //
	servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1    	    			    	  //
	servaddr.sin_port = htons(atoi(av[1])); 								    		    			    	// changer 8081 par atoi de av[1]
  
	// Binding newly created socket to given IP and verification 	                  //
	if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) == -1) // changer le != 0 pour == -1
		err(FATAL);
	if (listen(sockfd, 10) == -1) 												                          // changer le != 0 pour == -1
		err(FATAL);															                                    	// FIN COPIE 

	
	mini_serv();
}
