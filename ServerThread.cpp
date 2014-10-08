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

  if (loop_th.joinable() && loop_th.get_id() != std::this_thread::get_id())
    loop_th.join();

  if (connect_th.joinable() && connect_th.get_id() != std::this_thread::get_id())
    connect_th.join();

  std::lock_guard<std::mutex> lck(pms_mtx);
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
    throw std::string(gai_strerror(s));

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

  if (!gameState.addPlayer(playerId))
    throw std::string("Cannot create player for client.");

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
      timeout = INIT_TIMEOUT - (std::chrono::duration_cast<std::chrono::duration<double>>
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
  size_t nb_players, pms_size;
  struct pollfd pfd_listen;

  pfd_listen.fd = sockfd;
  pfd_listen.events = POLLIN;

  double elapsed = 0;
  std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

  do {
    if (poll(&pfd_listen, 1, 100) > 0) {
      acceptClient(-1);
    }

    elapsed = (std::chrono::duration_cast<std::chrono::duration<double>>
      (std::chrono::steady_clock::now() - begin).count());

    std::lock_guard<std::mutex> lck(pms_mtx);
    nb_players = std::max(gameState.getNbPlayers() - 1, 0);
    pms_size = pms.size();
  } while (running && pms_size < nb_players && elapsed <= CONNECT_TIMEOUT);

  close(sockfd);

  // Stop server if no connection made within timeout
  if (elapsed > CONNECT_TIMEOUT)
    ct.stopServer();
}

void ServerThread::acceptClient(int id) {
  struct pollfd pfd;
  pfd.fd = accept(sockfd, NULL, NULL);
  if (pfd.fd < 0) {
    std::cerr << "Error while accepting client\n";
    return;
  }

  pfd.events = POLLIN;
  fds.push_back(pfd);

  PlayerManager *pm = NULL;
  try {
    pm = new PlayerManager(pfd.fd, gameState, *this);
    pm->start(id);
    std::lock_guard<std::mutex> lck(pms_mtx);
    pms[pfd.fd] = pm;
  } catch (const std::string& e) {
    std::cerr << "Error: " << e << std::endl;
    delete pm;
  }
}

void ServerThread::loop() {
  char buf[BUFSIZE];
  ssize_t len;

  while (running) {
    if (poll(fds.data(), fds.size(), 100) > 0) {
      for (size_t i = 0; i < fds.size(); ++i) {
        if (!fds[i].revents)
          continue;

        std::lock_guard<std::mutex> lck(pms_mtx);
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

bool ServerThread::createBackupServer() {
  std::unique_lock<std::mutex> lck(new_srv_mtx);
  std::unique_lock<std::mutex> pms_lck(pms_mtx);
  std::map<int, PlayerManager*> old_pms(pms);
  pms_lck.unlock();

  for (const auto& pair: old_pms) {
    std::unique_lock<std::mutex> pms_lck_bis(pms_mtx);
    auto it = pms.find(pair.first);
    if (it == pms.end())
      continue;
    pms_lck_bis.unlock();

    new_srv_created = false;
    pair.second->createBackupServer();
    cv_new_srv.wait_for(lck, std::chrono::seconds(BACKUP_TIMEOUT));

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
    } catch (const std::string& e) {
    }
  }

  cv_new_srv.notify_one();

  if (new_srv_created) {
    lck.unlock();
    std::lock_guard<std::mutex> lck(pms_mtx);
    for (const auto& pair: pms) {
      if (pair.second != pm)
        pair.second->sendNewServer(host, port);
    }
  }
}

void ServerThread::syncMove(int id, char dir) {
  ct.syncMove(id, dir);
}
