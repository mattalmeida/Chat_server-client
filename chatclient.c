/*
** Matt Almeida
** CS372
** chatclient.c- Server program for simple chat on local host
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
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <signal.h>

#include <arpa/inet.h>

#define PORT "30021" // the port client will be connecting to 

#define MAXDATASIZE 500 // max number of bytes we can get at once 

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

int main(int argc, char *argv[])
{
    int sockfd, numbytes;  
    char buf[MAXDATASIZE];
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN];
	char handle[10];
	char message[MAXDATASIZE+10];
	struct sigaction quit;
	quit.sa_handler = intr_sig;
	sigemptyset(&quit.sa_mask);
	quit.sa_flags = 0;
	

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and connect to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("client: socket error\n");
            continue;
        }

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("client: connect error\n");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "client: failed to connect to server\n");
        return 2;
    }

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
            s, sizeof s);
    printf("client: connecting to %s\n", s);
	
	printf("Please enter your handle (within 10 chars): ");
	fgets(handle, sizeof(handle), stdin);
	handle[strlen(handle)-1] = '\0';
	numbytes = send(sockfd, handle, strlen(handle), 0);
	if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
        perror("recv\n");
        exit(1);
    }
	buf[numbytes] = '\0';
	printf("%s", buf);
	while(numbytes != 0){
		sigaction(SIGINT, &quit, NULL);
		printf("%s> ", handle);
		fgets(buf, sizeof(buf), stdin);
		sprintf(message, "%s> %s", handle, buf);
		message[strlen(message)-1] = '\0';
		numbytes = send(sockfd, message, strlen(message), 0);
		memset(buf, '\0', sizeof(buf));
		memset(message, '\0', sizeof(message));
		numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0);
		buf[numbytes] = '\0';
		printf("%s\n", buf);
	
	}
	
    freeaddrinfo(servinfo);


    close(sockfd);

    return 0;
}
