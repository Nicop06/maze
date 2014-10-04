#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <cstring>

#include <string>
#include <iostream>

#include "ClientThread.h"
#include "ServerThread.h"
#include "RemoteServer.h"
#include "ClientViewNcurses.h"
#include "GameState.h"
#include "Player.h"

ClientThread::ClientThread() : st(NULL), servers(2), id(-1), initialized(false),
pGameState(NULL), player(NULL), running(false) {
  view = new ClientViewNcurses(*this);
}

ClientThread::~ClientThread() {
  exit();
  delete view;
  delete st;

  for (const RemoteServer* serv: servers)
    delete serv;
}

void ClientThread::init(RemoteServer* serv) {
  if (servers.size() == 0) {
    addServer(serv, true);
    init();
  }
}

void ClientThread::init(ServerThread* st) {
  if (!st) {
    this->st = st;
    init();
  }
}

void ClientThread::init() {
  running = true;
  move(-1);
}

void ClientThread::loop() {
  createBackups();

  std::unique_lock<std::mutex> lck(cv_mtx);
  while (running) cv.wait(lck);
}

void ClientThread::addServer(RemoteServer* serv, bool join) {
  if (serv != NULL) {
    servers.insert(serv);
    if (join) {
      serv->join();
    } else {
      serv->connectSrv(id);
    }
  }
}

void ClientThread::delServer(RemoteServer* serv) {
  auto it = servers.find(serv);
  if (it != servers.end()) {
    servers.erase(it);
    delete serv;
  }

  createBackups();
}

const ServerThread* ClientThread::startServer(int N, const char* state, uint32_t size) {
  if (st) {
    try {
      st = new ServerThread(N, 0, *this);
      if (pGameState)
        pGameState->initState(state, size);
      st->init(NULL);
      st->connectClients();
      return st;
    } catch (const std::string& e) {
    }
  }

  return NULL;
}

void ClientThread::createBackups() {
  if (!st)
    return;

  while (servers.size() < NB_BACKUP) {
    if (!st->createServer()) {
      std::cerr << "Failed to create backup servers\n";
      exit();
    }
  }
}

void ClientThread::exit() {
  if (running) {
    for (RemoteServer* srv: servers)
      srv->exit();

    std::lock_guard<std::mutex> lck(cv_mtx);
    running = false;
    cv.notify_all();
  }
}

void ClientThread::move(char dir) {
  if (player) {
    player->move(dir);
    const std::string state = pGameState->getState();
    update(state.data(), state.size());
  } else if (servers.size() > 0) {
    for (auto it = servers.begin(); it != servers.end() && !(*it)->move(dir); ++it);
  }
}

void ClientThread::update(const char* state, const uint32_t size) {
  view->update(state, size);
}

void ClientThread::initView(int id, int N) {
  if (!initialized) {
    this->id = id;
    view->init(id, N);
    initialized = true;

    if (pGameState && !player)
      this->player = pGameState->addPlayer(id);
  }
}

void ClientThread::setState(GameState* gameState) {
  pGameState = gameState;

  if (pGameState)
    this->player = pGameState->getPlayer(id);
}
