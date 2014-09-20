#include <iostream>
#include <vector>
#include <chrono>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <cstring>

#include "ServerThread.h"
#include "PlayerManager.h"

bool ServerThread::running = false;

ServerThread::ServerThread(int N, int M) : gameState(N, M)  {
}

ServerThread::~ServerThread() {
  running = false;

  for (const auto& pair: pms) {
    PlayerManager* pm = pair.second;
    delete pm;
  }
}

void ServerThread::init(const char* port) {
  struct addrinfo hints;
  struct addrinfo *result, *rp;

  std::memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  int s = getaddrinfo(NULL, port, &hints, &result);
  if (s != 0)
    throw std::string("getaddrinfo: ", gai_strerror(s));

  // Try to connect to the list of addresses returned by getaddrinfo()

  for (rp = result; rp != NULL; rp = rp->ai_next) {
    sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);

    if (sockfd == -1)
      continue;

    if (bind(sockfd, rp->ai_addr, rp->ai_addrlen) == 0)
      break;

    close(sockfd);
  }

  if (rp == NULL)
    throw std::string("Couldn't open connection.");

  freeaddrinfo(result);

  if (listen(sockfd, BACKLOG) == -1)
    throw std::string("listen");

  gameState.initTreasures();

  running = true;
}

void ServerThread::acceptClients() {
  struct pollfd pfd_listen;

  pfd_listen.fd = sockfd;
  pfd_listen.events = POLLIN;

  std::cout << "Server: waiting for connections..." << std::endl;

  std::chrono::steady_clock::time_point begin;
  double timeout = -1;

  while ((timeout == -1 || timeout > 0) && running) {
    if (poll(&pfd_listen, 1, (int) (timeout * 1000)) > 0) {
      struct pollfd pfd;
      pfd.fd = accept(sockfd, NULL, NULL);
      if (pfd.fd < 0)
        throw std::string("accept");
      pfd.events = POLLIN | POLLHUP;
      fds.push_back(pfd);

      PlayerManager *pm = new PlayerManager(pfd.fd, gameState);
      pm->init();
      pms[pfd.fd] = pm;

      if (timeout == -1) {
        timeout = 0;
        begin = std::chrono::steady_clock::now();
      }
    }

    if (timeout != -1)
      timeout = TIMEOUT - (std::chrono::duration_cast<std::chrono::duration<double>>
        (std::chrono::steady_clock::now() - begin).count());
  }

  close(sockfd);
  std::cout << "Game starting..." << std::endl;
}

void ServerThread::loop() {
  char buf[BUFSIZE];
  int len;

  while (running) {
    int ret = poll(fds.data(), fds.size(), -1);    

    if (ret > 0) {
      for (uint32_t i = 0; i < fds.size(); ++i) {
        PlayerManager *pm = pms[fds[i].fd];
        if (pm) {
          if (fds[i].revents & POLLIN) {
            if ((len = recv(fds[i].fd, buf, BUFSIZE, 0)) == -1) {
              ServerThread::running = false;
              return;
            }

            std::string msg;
            msg.append(buf, len);
            pm->addMessage(msg);
          }

          if (fds[i].revents & POLLHUP) {
            pm->stop();
            delete pm;
          }
        }
      }
    }
  }
}
