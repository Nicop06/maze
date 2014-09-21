#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <cstring>
#include <poll.h>

#include <string>
#include <iostream>

#include "ClientThread.h"
#include "ClientViewNcurses.h"

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

  running = true;
}

void ClientThread::exit() {
  if (running) {
    const char msg[] = "exit";
    const int len = sizeof(msg);
    running = false;
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
  char buf[BUFSIZE];
  int len;

  if (!running)
    return;

  if (send(sockfd, "join", sizeof("join"), 0) == -1)
    exit();

  while (running && buffer.length() < 8) {
    if ((len = recv(sockfd, buf, BUFSIZE, 0)) == -1) {
      exit();
      return;
    }

    buffer.append(buf, len);
  }

  if (running) {
    int* data = (int*) buffer.data();
    int id = ntohl(*data);
    int N = ntohl(*(data+1));
    buffer.erase(0, 8);

    if (N > MAXSIZE) {
      exit();
      return;
    }

    view->init(id, N);
  }

  struct pollfd pfd;
  pfd.fd = sockfd;
  pfd.events = POLLIN | POLLHUP;

  while (running) {
    while ((len = view->update(buffer)) > 0)
      buffer.erase(0, len);

    if (poll(&pfd, 1, 100) > 0) {
      if (pfd.revents & POLLIN) {
        if ((len = recv(sockfd, buf, BUFSIZE, 0)) <= 0) {
          exit();
          return;
        }

        buffer.append(buf, len);
      }

      if (pfd.revents & POLLHUP) {
        exit();
        return;
      }
    }
  }
}

