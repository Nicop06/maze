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

ClientThread::ClientThread() : st(NULL), id(-1),
  initialized(false), pGameState(NULL), running(false), state_owner(false) {
  view = new ClientViewNcurses(*this);
}

ClientThread::~ClientThread() {
  stopServer();

  std::unique_lock<std::mutex> serv_lck(servers_mtx);
  std::set<RemoteServer*> servers_bis;
  servers_bis.swap(servers);
  serv_lck.unlock();

  for (RemoteServer* serv : servers_bis)
    delete serv;
  servers.clear();

  delete view;
}

void ClientThread::init(RemoteServer* serv) {
  std::unique_lock<std::mutex> lck(servers_mtx);
  if (servers.size() == 0) {
    lck.unlock();
    running = true;
    addServer(serv, true);
    move(-1);
  }
}

void ClientThread::init(ServerThread* st) {
  if (!this->st) {
    this->st = st;
    running = true;
    state_owner = true;
    move(-1);
  }
}

void ClientThread::loop() {
  std::unique_lock<std::mutex> lck(loop_mtx);
  while (running) {
    createBackups();
    if (running)
      cv_loop.wait(lck);
  }
}

void ClientThread::addServer(RemoteServer* serv, bool join) {
  if (serv != NULL && running) {
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
  _delServer(serv);
}

void ClientThread::_delServer(RemoteServer* serv) {
  auto it = servers.find(serv);

  if (it != servers.end()) {
    delete *it;
    servers.erase(it);
  }

  cv_loop.notify_one();
}

const ServerThread* ClientThread::startServer(int N, const char* state, size_t size) {
  if (!st) {
    ServerThread *st(NULL);
    try {
      st = new ServerThread(N, 0, *this);
      if (pGameState)
        pGameState->initState(state, size);
      st->init(NULL);
      st->connectClients();
      this->st = st;
      std::lock_guard<std::mutex> state_lck(state_mtx);
      state_owner = false;
      return st;
    } catch (const std::string& e) {
      std::cerr << "Error: " << e << std::endl;
      delete st;
    }
  }

  return NULL;
}

void ClientThread::stopServer() {
  if (!st)
    return;
  ServerThread* old_st = st;
  st = NULL;
  pGameState = NULL;
  std::unique_lock<std::mutex> state_lck(state_mtx);
  state_owner = false;
  state_lck.unlock();

  delete old_st;
}

void ClientThread::createBackups() {
  if (!st)
    return;

  std::unique_lock<std::mutex> lck(servers_mtx);
  while (servers.size() < NB_BACKUP) {
    lck.unlock();
    if (!st->createBackupServer()) {
      std::cerr << "Failed to create backup servers\n";
      stop();
      return;
    }
    lck.lock();
  }
}

void ClientThread::stop() {
  if (running) {
    running = false;
    cv_loop.notify_one();
  }
}

void ClientThread::move(char dir) {
  if (pGameState && st) {
    syncMove(id, dir);
    const std::string state = pGameState->getState();
    update(state.data(), state.size());
  } else {
    std::lock_guard<std::mutex> lck(servers_mtx);
    if (servers.size() > 0) {
      for (RemoteServer* serv: servers) {
        if (serv->move(dir)) {
          break;
        } else {
          _delServer(serv);
        }
      }
    }
  }
}

void ClientThread::syncMove(int id, char dir) {
  std::lock_guard<std::mutex> lck(sync_move_mtx);
  if (pGameState) {
    std::unique_lock<std::mutex> state_lck(state_mtx);

    if (!state_owner) {
      std::unique_lock<std::mutex> srv_lck(servers_mtx);
      nb_owner = 0;
      for (RemoteServer *serv : servers) {
        if (serv->requestStateOwnership()) {
          nb_owner++;
        } else {
          _delServer(serv);
        }
      }
      srv_lck.unlock();

      std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
      while (nb_owner > 0 || !state_owner) {
        if (cv_state.wait_for(state_lck, begin - std::chrono::steady_clock::now()
              + std::chrono::seconds(LOCK_TIMEOUT)) == std::cv_status::timeout) {
          break;
        }
      }

      nb_owner = 0;
      state_owner = true;
    }

    pGameState->move(id, dir);
    sendSyncMove(id, dir);
  }
}

void ClientThread::movePlayer(int id, char dir) {
  if (pGameState)
    pGameState->move(id, dir);
}

bool ClientThread::releaseState() {
  std::lock_guard<std::mutex> lck(state_mtx);
  if (state_owner) {
    state_owner = false;
    return true;
  } else {
    return false;
  }
}

void ClientThread::stateAcquired(bool acquired) {
  std::lock_guard<std::mutex> state_lck(state_mtx);
  if (state_owner || !st)
    return;

  if (acquired)
    state_owner = true;

  nb_owner--;
  cv_state.notify_one();
}

void ClientThread::sendSyncMove(int id, char dir) {
  std::unique_lock<std::mutex> lck(sync_mtx);
  nb_sync = 0;

  std::unique_lock<std::mutex> srv_lck(servers_mtx);
  for (RemoteServer* serv: servers) {
    if (serv->movePlayer(id, dir)) {
      ++nb_sync;
    } else {
      _delServer(serv);
    }
  }
  srv_lck.unlock();

  std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
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

void ClientThread::update(const char* state, size_t size) {
  if (initialized)
    view->update(state, size);
}

void ClientThread::initView(int id, int N) {
  if (!initialized) {
    this->id = id;
    view->init(id, N);
    initialized = true;
  }
}

void ClientThread::setState(GameState* gameState) {
  pGameState = gameState;
}
