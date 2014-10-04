#include <iostream>
#include <vector>
#include <chrono>
#include <algorithm>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <cstring>

#include "ServerThread.h"

ServerThread::ServerThread(int N, int M, ClientThread& client) : gameState(N, M), ct(client), running(false) {
  ct.setState(&gameState);
}

ServerThread::~ServerThread() {
  running = false;

  if (loop_th.joinable())
    loop_th.join();

  for (const auto& pair: pms)
    delete pair.second;
}

void ServerThread::init(const char* port) {
  bool ret = false;

  if (port) {
    this->port = port;
    ret = tryBind(port);
  } else {
    bool ret = false;
    for (int p = PORT_START; p <= PORT_END && !ret; ++p) {
      this->port = std::to_string(p);
      ret = tryBind(this->port.c_str());
    }
  }

  if (!ret)
    throw std::string("connection impossible");

  if (listen(sockfd, BACKLOG) == -1)
    throw std::string("listen");

  gameState.initTreasures();

  running = true;
}

bool ServerThread::tryBind(const char* port) {
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

  freeaddrinfo(result);

  return rp;
}

void ServerThread::acceptClients() {
  struct pollfd pfd_listen;
  int playerId = 0, ctId = ++playerId;

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
      pfd.events = POLLIN;
      fds.push_back(pfd);

      PlayerManager *pm = new PlayerManager(pfd.fd, gameState);
      pm->init(++playerId);
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
  //chooseBackup();
  std::cout << "Game starting..." << std::endl;
  ct.initView(ctId, gameState.getSize());
  loop_th = std::thread(&ServerThread::loop, this);
}

void ServerThread::loop() {
  char buf[BUFSIZE];
  int len;

  while (running) {
    if (poll(fds.data(), fds.size(), 100) > 0) {
      for (uint32_t i = 0; i < fds.size(); ++i) {
        if (!fds[i].revents)
          continue;

        auto it = pms.find(fds[i].fd);

        if (it != pms.end()) {
          PlayerManager* pm = it->second;
          if ((len = recv(fds[i].fd, buf, BUFSIZE, MSG_DONTWAIT)) == -1) {
            running = false;
            return;
          }

          if (len == 0) {
            delete pm;
            pms.erase(fds[i].fd);
            if (pms.size() == 0) {
              running = false;
              return;
            }
          }

          std::string msg;
          msg.append(buf, len);
          pm->addMessage(msg);
        }
      }

      //remove closed sockets from fds
      fds.erase(std::remove_if(fds.begin(), fds.end(), [=](struct pollfd pfd){ return pms.find(pfd.fd) == pms.end(); }), fds.end());
    }
  }
}
