#include <iostream>
#include <unistd.h>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "ClientThread.h"

#define PORT "3490" // the port client will be connecting to 
#define MAXDATASIZE 100 // max number of bytes we can get at once 

using namespace std;

// get sockaddr, IPv4 or IPv6:
void* ClientThread::get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

ClientThread::ClientThread() {}

ClientThread::~ClientThread() {}

void ClientThread::startConnection(int argc, char* argv[]){
	int sockfd, numbytes;  
	char buf[MAXDATASIZE];
	struct addrinfo hints, *servinfo, *p;
	int rv;
	char s[INET6_ADDRSTRLEN];

	if (argc < 2) {
		cerr << "usage: client hostname" << endl;
		return;
	}

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((rv = getaddrinfo(argv[1], PORT, &hints, &servinfo)) != 0) {
		throw string("getaddrinfo: ", gai_strerror(rv));
	}

	// loop through all the results and connect to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,p->ai_protocol)) == -1) {
			cerr << "client: socket" << endl;
			continue;
		}

		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			cerr << "client: connect" << endl;
			continue;
		}
		break;//we found a socket
	}

	if (p == NULL) {
		throw string("client: failed to connect");
	}
	
	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),s, sizeof(s)); //put server's IP address into char[] s
	cout << "client: connecting to " << s << endl;

	freeaddrinfo(servinfo); // all done with this structure
	
	//first message reception from the server
	if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
		throw string("recv");
	}

	buf[numbytes] = '\0';
	cout << "client: received '" << buf << "'" << endl;

	while(true){
		cin.getline(buf, MAXDATASIZE);
		
		if (send(sockfd, buf, strlen(buf)+1, 0) == -1){ //we send the content of buf
			close(sockfd);
			throw string("send");
		}
		if(strcmp(buf,"exit")==0){
			cout << "Exiting connection" << endl;
			break; //if "exit", we exit the loop
		}

		if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) { //we receive the server's message
			close(sockfd);
			throw string("recv");
		}

		buf[numbytes] = '\0';
		cout << "client: received '" << numbytes << " bytes: " << buf << "'" << endl;
	}	

	close(sockfd);

}
