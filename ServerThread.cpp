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
#include "PlayerManager.h"
#include "RemoteServer.h"

ServerThread::ServerThread(int N, int M, ClientThread& client) : gameState(N, M), ct(client), running(false) {
  ct.setState(&gameState);
}

ServerThread::~ServerThread() {
  running = false;

  if (loop_th.joinable())
    loop_th.join();

  if (connect_th.joinable())
    connect_th.join();

  for (const auto& pair: pms)
    delete pair.second;
}

void ServerThread::init(const char* port) {
  if (running)
    return;

  bool ret = false;

  if (port) {
    this->port = port;
    ret = tryBind(port);
  } else {
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
      acceptClient(++playerId);

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
  ct.initView(ctId, gameState.getSize());
  loop_th = std::thread(&ServerThread::loop, this);
}

void ServerThread::connectClients() {
  connect_th = std::thread(&ServerThread::connectClientsLoop, this);
  loop_th = std::thread(&ServerThread::loop, this);
}

void ServerThread::connectClientsLoop() {
  uint32_t nb_players;
  struct pollfd pfd_listen;

  pfd_listen.fd = sockfd;
  pfd_listen.events = POLLIN;

  do {
    if (poll(&pfd_listen, 1, 100) > 0) {
      acceptClient(-1);
    }

    nb_players = std::max(gameState.getNbPlayers() - 1, 0);
  } while (running && pms.size() < nb_players);

  close(sockfd);
}

void ServerThread::acceptClient(int id) {
  struct pollfd pfd;
  pfd.fd = accept(sockfd, NULL, NULL);
  if (pfd.fd < 0)
    throw std::string("accept");
  pfd.events = POLLIN;
  fds.push_back(pfd);

  PlayerManager *pm = new PlayerManager(pfd.fd, gameState, *this);
  pm->init(id);
  pms[pfd.fd] = pm;
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

bool ServerThread::createServer() {
  std::unique_lock<std::mutex> lck(new_srv_mtx);

  for (const auto& pair: pms) {
    new_srv_created = false;
    pair.second->createServer();
    cv_new_srv.wait_for(lck, std::chrono::seconds(LOCK_TIMEOUT));

    if (new_srv_created)
      break;
  }

  return new_srv_created;
}

void ServerThread::newServer(const PlayerManager* pm, const std::string& host, const std::string& port) {
  std::unique_lock<std::mutex> lck(new_srv_mtx);
  if (new_srv_created)
    return;

  new_srv_created = false;

  if (host.size() > 0 && port.size() > 0) {
    RemoteServer* serv = new RemoteServer(ct);
    try {
      serv->init(host.c_str(), port.c_str());
      ct.addServer(serv);
      new_srv_created = true;
    } catch (std::string& e) {
    }
  }

  cv_new_srv.notify_one();

  if (new_srv_created) {
    lck.unlock();
    for (const auto& pair: pms) {
      if (pair.second != pm)
        pair.second->sendServer(host, port);
    }
  }
}

void ServerThread::syncMove(Player* player, char dir) {
  ct.syncMove(player, dir);
}
