#include <iostream>
#include <thread>
#include <algorithm>

#include <cstdlib>
#include <unistd.h>
#include <cstring>
#include <sys/socket.h>
#include <sys/select.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "ServerThread.h"

#define PORT "3490" // the port client will be connecting to 
#define BACKLOG 10
#define MAXDATASIZE 100 // max number of bytes we can get at once 

using namespace std;

void* ServerThread::get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

ServerThread::ServerThread() {}

ServerThread::~ServerThread() {}

void ServerThread::initConnection(){
	struct addrinfo hints, *servinfo, *p;
	int yes=1;
	int rv;
	
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC; // use IPv4 or IPv6 (use AF_INET for IPv4  or AF_INET6 for IPv6)
	hints.ai_socktype = SOCK_STREAM; //TCP
	hints.ai_flags = AI_PASSIVE; // use my IP address

	if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
		throw string("getaddrinfo: ", gai_strerror(rv));
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			cerr << "socket" << endl;
			continue;
		}
		
		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
			cerr << "setsockopt" << endl;
			continue;
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			cerr << "bind" << endl;
			continue;
		}

    		break; // if we get here, we must have connected successfully
	}

	if (p == NULL) {
    	// looped off the end of the list with no successful bind
		throw string("failed to bind socket");
	}

	cout << "Local socket bound to port" << endl;
	freeaddrinfo(servinfo); // all done with this structure
	
	if (listen(sockfd, BACKLOG) == -1) {
		throw string("listen");
	}
}

void ServerThread::acceptConnections(){
	struct timeval tv; //timeval for timeout in select
	fd_set readfds; //file descriptor set of readable sockets
	
	
	FD_ZERO(&readfds); //clear fd set
	FD_SET(sockfd, &readfds); //add listening server socket to the fd set

	tv.tv_sec = 20;
	tv.tv_usec = 0;

	//faire un select avec timeout(temps_restant) et un while(temps_restant>0);
	cout << "server: waiting for connections..." << endl;
	
	//accepting first connection
	acceptInSock();

	cout << "Countdown begins" << endl;

	while(tv.tv_sec>0 || tv.tv_usec>0){
		if (select(sockfd+1, &readfds, NULL, NULL, &tv) == -1) {
			throw string("select");		
		}
    cout << tv.tv_sec;

		if(FD_ISSET(sockfd,&readfds)){
			//incoming new socket waiting for accept()
			acceptInSock();
		}else{
			//time out or
			//incoming socket waiting for recv() : already handled by a thread
			FD_ZERO(&readfds); //clear fd set
			FD_SET(sockfd, &readfds); //add listening server socket to the fd set	
		}
	}

	close(sockfd);
	cout << "Timeout elapsed" << endl;

	vect_mutex.lock();
	cout << inSockFds.size() << " client(s) connected" << endl;
	vect_mutex.unlock();

	while(true){} //à modifier
}

void ServerThread::acceptInSock(){
	int newfd; //newfd : incoming socket
	struct sockaddr_storage their_addr; // connector's address information
	char s[INET6_ADDRSTRLEN];
	socklen_t sin_size;
	thread t;
	
	sin_size = sizeof(their_addr);
	newfd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
	if (newfd == -1) {
		throw string("accept");
	}

	inet_ntop(their_addr.ss_family,	get_in_addr((struct sockaddr *)&their_addr),s, sizeof(s));
	cout << "server: got connection from " << s << endl;
		
	vect_mutex.lock();
	inSockFds.push_back(newfd);
	vect_mutex.unlock();

	t = thread(&ServerThread::echoing,this,newfd);
	t.detach();
}

void ServerThread::echoing(int inSockFd){
	char buf[MAXDATASIZE];
	int numbytes;
	vector<int>::iterator it;

	if (send(inSockFd, "Hello, world!", 13, 0) == -1){
		close(inSockFd);
		throw string("send");
	}

	while(true){
		if((numbytes = recv(inSockFd, buf, MAXDATASIZE-1, 0)) == -1){
			close(inSockFd);
			throw string("recv");
		}
		if(numbytes==0){
			cout << "Client exited connection" << endl;
			break;
		}

		buf[numbytes] = '\0';
		if(strcmp(buf,"exit")==0){
			cout << "Client exited connection" << endl;
			break;
		}
		if (send(inSockFd, buf, numbytes, 0) == -1){
			close(inSockFd);
			throw string("send");
		}
		
	}

	close(inSockFd);

	vect_mutex.lock();
	it = find(inSockFds.begin(), inSockFds.end(), inSockFd);
	inSockFds.erase(it);
	vect_mutex.unlock();
}
