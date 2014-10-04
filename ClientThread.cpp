#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <cstring>

#include <string>
#include <iostream>
#include <chrono>

#include "ClientThread.h"
#include "ServerThread.h"
#include "RemoteServer.h"
#include "ClientViewNcurses.h"
#include "GameState.h"
#include "Player.h"

ClientThread::ClientThread() : st(NULL), servers(2), old_servers(2), id(-1),
  initialized(false), pGameState(NULL), player(NULL), running(false) {
  view = new ClientViewNcurses(*this);
}

ClientThread::~ClientThread() {
  exit();
  delete view;
  delete st;

  cleanServers();

  std::lock_guard<std::mutex> lck(servers_mtx);
  for (const RemoteServer* serv: servers)
    delete serv;
}

void ClientThread::init(RemoteServer* serv) {
  std::unique_lock<std::mutex> lck(servers_mtx);
  if (servers.size() == 0) {
    lck.unlock();
    addServer(serv, true);
    init();
  }
}

void ClientThread::init(ServerThread* st) {
  if (!this->st) {
    this->st = st;
    init();
  }
}

void ClientThread::init() {
  running = true;
  move(-1);
}

void ClientThread::loop() {
  std::unique_lock<std::mutex> lck(loop_mtx);
  while (running) {
    cleanServers();
    createBackups();
    cv_loop.wait(lck);
  }
}

void ClientThread::addServer(RemoteServer* serv, bool join) {
  if (serv != NULL) {
    bool ret;
    if (join) {
      ret = serv->join();
    } else {
      ret = serv->connectSrv(id);
    }

    if (!ret) {
      delete serv;
      return;
    }

    std::lock_guard<std::mutex> lck(servers_mtx);
    servers.insert(serv);
  }
}

void ClientThread::delServer(RemoteServer* serv) {
  std::unique_lock<std::mutex> lck(servers_mtx);
  delSrv(serv);
}

void ClientThread::delSrv(RemoteServer* serv) {
  auto it = servers.find(serv);
  if (it != servers.end()) {
    servers.erase(it);
    old_servers.insert(serv);
  }

  cv_loop.notify_one();
}

void ClientThread::cleanServers() {
  std::lock_guard<std::mutex> lck(servers_mtx);
  for (RemoteServer* serv: old_servers) {
    old_servers.erase(serv);
    delete serv;
  }
}

const ServerThread* ClientThread::startServer(int N, const char* state, uint32_t size) {
  if (!st) {
    try {
      st = new ServerThread(N, 0, *this);
      if (pGameState)
        pGameState->initState(state, size);
      st->init(NULL);
      st->connectClients();
      return st;
    } catch (const std::string& e) {
      std::cerr << "Error: " << e << std::endl;
    }
  }

  return NULL;
}

void ClientThread::createBackups() {
  if (!st)
    return;

  std::unique_lock<std::mutex> lck(servers_mtx);
  while (servers.size() < NB_BACKUP) {
    lck.unlock();
    if (!st->createServer()) {
      std::cerr << "Failed to create backup servers\n";
      exit();
      return;
    }
    lck.lock();
  }
}

void ClientThread::exit() {
  if (running) {
    std::lock_guard<std::mutex> lck(servers_mtx);
    for (RemoteServer* srv: servers)
      srv->exit();

    std::lock_guard<std::mutex> loop_lck(loop_mtx);
    running = false;
    cv_loop.notify_all();
  }
}

void ClientThread::move(char dir) {
  if (player) {
    syncMove(player, dir);
    const std::string state = pGameState->getState();
    update(state.data(), state.size());
  } else {
    std::lock_guard<std::mutex> lck(servers_mtx);
    if (servers.size() > 0) {
      for (RemoteServer* serv: servers) {
        if (serv->move(dir)) {
          break;
        } else {
          delSrv(serv);
        }
      }
    }
  }
}

void ClientThread::movePlayer(int id, char dir) {
  Player* player = pGameState->getPlayer(id);
  if (player)
    player->move(dir);
}

void ClientThread::syncMove(Player* player, char dir) {
  if (player)
    player->move(dir, std::bind(&ClientThread::sendSync, this, player->id(), dir));
}

void ClientThread::sendSync(int id, char dir) {
  std::unique_lock<std::mutex> lck(sync_mtx);
  nb_sync = 0;

  std::unique_lock<std::mutex> srv_lck(servers_mtx);
  for (RemoteServer* serv: servers) {
    if (serv->movePlayer(id, dir)) {
      ++nb_sync;
    } else {
      delSrv(serv);
    }
  }
  srv_lck.unlock();

  std::chrono::steady_clock::time_point begin;
  while (nb_sync > 0) {
    if (cv_sync.wait_for(lck, begin - std::chrono::steady_clock::now()
          + std::chrono::seconds(LOCK_TIMEOUT)) == std::cv_status::timeout)
      return;
  }
}

void ClientThread::moveDone() {
  std::lock_guard<std::mutex> lck(sync_mtx);
  nb_sync--;
  cv_sync.notify_one();
}

void ClientThread::update(const char* state, const uint32_t size) {
  if (initialized)
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
