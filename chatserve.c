/*
** Matt Almeida
** CS372
** chatserve.c- Server program for simple chat on local host
** Thanks to beej's guide to Network Programming at beej.us
** for a great guide and lots of sample code
** Tested on access.engr.oregonstate.edu
*/
#define _POSIX_SOURCE
#define _BSD_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#define PORT "30021"  // the port users will be connecting to

#define BACKLOG 10     // how many pending connections queue will hold

#define MAXDATASIZE 500 // max number of bytes we can get at once

void sigchld_handler(int s)
{
    while(waitpid(-1, NULL, WNOHANG) > 0);
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void intr_sig(int signal){
    exit(0);
	return;
}


int main(void)
{
    int sockfd, connect_fd;
	int numbytes;
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    struct sigaction sa;
    int yes=1;
    char s[INET6_ADDRSTRLEN];
    int rv;
	char handle[11] = "Mattchat> \0";
	char usrhandle[10];
	int c;
	char initmessage[73];
	char buf[MAXDATASIZE];
	char message[MAXDATASIZE+11];
	struct sigaction quit;
	quit.sa_handler = intr_sig;
	sigemptyset(&quit.sa_mask);
	quit.sa_flags = 0;

	

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use local IP

    if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("server: socket\n");
            continue;
        }
		
		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
			perror("setsockopt");
			exit(1);
		} 

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind\n");
            continue;
        }

        break;
    }

    if (p == NULL)  {
        fprintf(stderr, "server: failed to bind\n");
        return 2;
    }

    freeaddrinfo(servinfo);

    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen\n");
        exit(1);
    }

    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction\n");
        exit(1);
    }

    printf("server: waiting for connections...\n");
	printf("-Use keyboard interrupt to exit-\n");

    while(1) {
        sin_size = sizeof(their_addr);
        connect_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (connect_fd == -1) {
            perror("accept\n");
            continue;
        }

        inet_ntop(their_addr.ss_family,
            get_in_addr((struct sockaddr *)&their_addr),
            s, sizeof s);
        printf("server: got connection from %s\n", s);

        if (!fork()) { 
			//child process
            close(sockfd);
			if ((numbytes = recv(connect_fd, buf, MAXDATASIZE-1, 0)) == -1) {
				perror("recv\n");
				exit(1);
			}
			memset(usrhandle, '\0', sizeof(usrhandle));
			for(c = 0; c < numbytes; c++){
				usrhandle[c] = buf[c];
			}
			printf("Connected with %s\n", usrhandle);
			sprintf(initmessage, "%sHello, %s, begin your chat session\n-Use keyboard interrupt to exit-\n", handle, usrhandle);
            if (send(connect_fd, initmessage, strlen(initmessage), 0) == -1)
                perror("send\n");
			
			while(numbytes != 0){
				sigaction(SIGINT, &quit, NULL);
				memset(buf, '\0', sizeof(buf));
				numbytes = recv(connect_fd, buf, MAXDATASIZE-1, 0);
				buf[numbytes] = '\0';
				printf("%s\n", buf);
				printf("%s", handle);
				memset(buf, '\0', sizeof(buf));
				memset(message, '\0', sizeof(message));
				fgets(buf, sizeof(buf), stdin);
				sprintf(message, "%s%s", handle, buf);
				message[strlen(message)-1] = '\0';
				send(connect_fd, message, strlen(message), 0);
				memset(buf, '\0', sizeof(buf));
				
			}
			
            close(connect_fd);
            exit(0);
        }
        close(connect_fd);
    }

    return 0;
}