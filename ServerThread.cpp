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
#include "ClientThread.h"

bool ServerThread::running = false;

ServerThread::ServerThread(int N, int M, ClientThread* client) : gameState(N, M), ct(client){
}

ServerThread::~ServerThread() {
  running = false;

  for (const auto& pair: pms) {
    PlayerManager* pm = pair.second;
    delete pm;
  }
}

void ServerThread::init(const char* p, const char* servP) {
  port = p;
  servPort = servP;

  struct addrinfo hints;
  struct addrinfo *result, *rp;

  std::memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  int s = getaddrinfo(NULL, port.c_str(), &hints, &result);
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
  int playerId = 0;
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
  std::thread(&ServerThread::waitClientsJoin, this).detach();
}

void ServerThread::waitClientsJoin(){
  std::cout << "Clients are joining..." << std::endl;
  
  //wait for all clients to join
  for(auto it = pms.begin(); it!=pms.end(); ++it){
    std::thread(&PlayerManager::waitForJoin, it->second).join();
  }
  
  clientId = ct->getId();
  std::cout << "Local client is " << clientId << std::endl;
  chooseBackup();

  std::cout << "Game starting..." << std::endl;
  for(auto it = pms.begin(); it!=pms.end(); ++it){
    it->second->start();
  }
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

void ServerThread::chooseBackup(){
  unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
  std::minstd_rand generator(seed);
  std::uniform_int_distribution<unsigned int> distribution(0, fds.size()-1);
  std::string ip;
  unsigned int i = distribution(generator);
  PlayerManager* pm = pms[fds[i].fd];
  if(fds.size()>1){
    while(pm->getPlayerId()!= clientId && !pm->sendBackup(servPort)){
      i = distribution(generator);
      pm = pms[fds[i].fd];
    }
    ip = pm->getIpAddr();
    for(unsigned int j=0; j<fds.size(); j++){
      if(j!=i){
        pms[fds[j].fd]->sendBackupIp(ip, port);
      }
    }
  }else{
    //To be modified
    running = false;
    std::cerr << "Only one player!" << std::endl;
    return;
  }
}
