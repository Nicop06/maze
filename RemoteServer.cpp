#include <poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <cstring>
#include <string>

#include "RemoteServer.h"

RemoteServer::~RemoteServer() {
  exit();

  if (loop_th.joinable())
    loop_th.join();

  close(sockfd);
}

void RemoteServer::init(const char* host, const char* port) {
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
  loop_th = std::thread(&RemoteServer::loop, this);
}

void RemoteServer::exit() {
  if (running) {
    running = false;
    const char msg[] = "exit";
    const int len = sizeof(msg);
    send(sockfd, msg, len, 0);
  }
}

void RemoteServer::loop() {
  char buf[BUFSIZE];
  std::string buffer;
  uint32_t size(0), len, head;
  int* data;
  struct pollfd pfd;

  pfd.fd = sockfd;
  pfd.events = POLLIN | POLLHUP;

  while (running) {
    if (poll(&pfd, 1, 100) > 0) {
      if ((len = recv(sockfd, buf, BUFSIZE, MSG_DONTWAIT)) <= 0) {
        exit();
        return;
      }
      buffer.append(buf, len);
    }

    if (size == 0 && buffer.size() >= 8) {
      data = (int*) buffer.data();
      head = ntohl(data[0]);
      size = ntohl(data[1]);
    }

    if (size > 0 && buffer.size() >= size + 8) {
      data = (int*) (buffer.data() + 8);
      switch (head) {
        case INIT:
          ct.initView(ntohl(data[0]), ntohl(data[1]));
          break;
        case STATE:
          ct.update(buffer.data() + 8, size);
          break;
      }

      buffer.erase(0, size + 8);
      size = 0;
    }
  }
}

void RemoteServer::move(char dir) {
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

void RemoteServer::join() {
  const char msg[] = "join";
  if (send(sockfd, msg, sizeof(msg), 0) == -1)
    exit();
}