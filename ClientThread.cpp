#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <cstring>

#include <string>
#include <iostream>

#include "ClientThread.h"
#include "ClientViewNcurses.h"
#include "config.h"

ClientThread::ClientThread() {
  view = new ClientViewNcurses(*this);
}

ClientThread::~ClientThread() {
  exit();
  delete view;
}

void ClientThread::init(const char* host, const char* port) {
  struct addrinfo hints, *result, *rp;

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  int s = getaddrinfo(host, port, &hints, &result);
  if (s != 0)
    throw std::string("getaddrinfo: ", gai_strerror(s));

  // Try to connect to the list of addresses returned by getaddrinfo()

  for (rp = result; rp != NULL; rp = rp->ai_next) {
    sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if (sockfd == -1)
      continue;

    if (connect(sockfd, rp->ai_addr, rp->ai_addrlen) != -1)
      break;

    close(sockfd);
  }

  if (rp == NULL)
    throw std::string("Couldn't connect to server.");

  freeaddrinfo(result);

  pfd.fd = sockfd;
  pfd.events = POLLIN | POLLHUP;

  running = true;
  
}

void ClientThread::exit() {
  if (running) {
    running = false;
    const char msg[] = "exit";
    const int len = sizeof(msg);
    send(sockfd, msg, len, 0);
    close(sockfd);
  }
}

void ClientThread::move(char dir) {
  std::string msg;

  switch (dir) {
    case 'N':
    case 'E':
    case 'W':
    case 'S':
      msg += dir;
      break;

    case 0:
      return;

    default:
      msg = "NoMove";
      break;
  }

  if (send(sockfd, msg.data(), msg.length() + 1, 0) == -1)
    exit();
}

void ClientThread::loop() {
  //séparer en init() et loop()
  int len, head, id, N;
  int* data;
  if (!running)
    return;

  if (send(sockfd, "join", sizeof("join"), 0) == -1)
    exit();

  // Wait for player id and game size
  while (running && buffer.length() < 3*sizeof(int))
    read();

  if (running) {
    data = (int*) buffer.data();
    head = ntohl(*data);
    id = ntohl(*(data+1));
    N = ntohl(*(data+2));
    buffer.erase(0, 3*sizeof(int));

    if (N > MAXSIZE || head!=INIT) {
      exit();
      return;
    }
    
    while (running && buffer.length() < sizeof(int))
      read();

    data = (int*) buffer.data();
    head = ntohl(*data);
    if(head==BACKUP){
      //initialize backup server and retrieve the port for connexion with the server
      //connect to server
      //wait for other clients to connect to the created backup server
      int portSize = ntohl(*(data+1));
      std::string port = buffer.substr(2*sizeof(int), portSize);
      std::cout << "I am the backup server, port to server: " << port << std::endl;
      buffer.erase(0, 2*sizeof(int)+portSize);
    }else if(head==BACK_IP){
      //connect with backup server
      int ipSize = ntohl(*(data+1));
      int portSize = ntohl(*(data+2));
      std::string ip = buffer.substr(3*sizeof(int), ipSize);
      std::string port = buffer.substr(3*sizeof(int)+ipSize, portSize);
      buffer.erase(0, 3*sizeof(int)+ipSize+portSize);
      std::cout << "I am not the backup server, id: " << id << ", backup server ip: " << ip << ", port: " << port << std::endl;
    }else{
      std::cout << "Hum, embarassing, id: " << id << std::endl;
      buffer.erase(0, sizeof(int));
      exit();
      return;
    }
    
    view->init(id, N);
  }

  // Wait for new messages
  while (running) {
    read();
    buffer.erase(0,sizeof(int));//remove head
    while ((len = view->update(buffer)) > 0)
      buffer.erase(0, len);
  }
}

void ClientThread::read() {
  int len;

  if (poll(&pfd, 1, 100) > 0) {
    if ((len = recv(sockfd, buf, BUFSIZE, MSG_DONTWAIT)) <= 0) {
      exit();
      return;
    }
    buffer.append(buf, len);
  }
}
