#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/types.h>

#include "ServerThread.h"

#define PORT "3490" // the port client will be connecting to 
#define BACKLOG 10
#define MAXDATASIZE 100 // max number of bytes we can get at once 

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

ServerThread::ServerThread() {}

ServerThread::~ServerThread() {}

void ServerThread::startConnection(){
	int sockfd, newfd; //sockfd : listening socket, newfd : incoming socket
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr; // connector's address information
	char s[INET6_ADDRSTRLEN];
	char buf[MAXDATASIZE];
	socklen_t sin_size;
	int yes=1;
	int rv;
	int numbytes;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC; // use IPv4 or IPv6 (use AF_INET for IPv4  or AF_INET6 for IPv6)
	hints.ai_socktype = SOCK_STREAM; //TCP
	hints.ai_flags = AI_PASSIVE; // use my IP address

	if ((rv = getaddrinfo(NULL, "3490", &hints, &servinfo)) != 0) {
	    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
	    exit(1);
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			perror("socket");
			continue;
		}
		
		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
			perror("setsockopt");
			exit(1);
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("bind");
			continue;
		}

    		break; // if we get here, we must have connected successfully
	}

	if (p == NULL) {
    	// looped off the end of the list with no successful bind
    		fprintf(stderr, "failed to bind socket\n");
    		exit(2);
	}

	printf("Local socket bound to port\n");
	freeaddrinfo(servinfo); // all done with this structure
	
	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}

	printf("server: waiting for connections...\n");
	
	sin_size = sizeof(their_addr);
	newfd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
	if (newfd == -1) {
		perror("accept");
		exit(1);
	}

	inet_ntop(their_addr.ss_family,	get_in_addr((struct sockaddr *)&their_addr),s, sizeof(s));
	printf("server: got connection from %s\n", s);
	
	if (send(newfd, "Hello, world!", 13, 0) == -1){
		perror("send");
		exit(1);
	}

	while(1){
		numbytes = recv(newfd, buf, MAXDATASIZE-1, 0);
		if(recv == -1){
			perror("recv");
			break;
		}
		buf[numbytes] = '\0';
		if (send(newfd, buf, numbytes, 0) == -1){
			perror("send");
			break;
		}
		if(strcmp(buf,"exit")==0){
			printf("Client exited connection\n");
			break;
		}
	}

	close(newfd);
	close(sockfd);

}
