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

ClientThread::ClientThread() : st(NULL), serv(NULL), id(-1), pGameState(NULL), player(NULL), running(false) {
  view = new ClientViewNcurses(*this);
}

ClientThread::~ClientThread() {
  exit();
  delete view;
  delete serv;
  delete st;
}

void ClientThread::initClientServer(int N, int M, const char* port, const char* servPort){
  st = new ServerThread(N, M, *this);
  st->init(port,servPort);
  st->acceptClients();
  move(-1);
  st->wait();
}

void ClientThread::initClient(const char* host, const char* port){
  init(host, port);
  serv->wait();
}

void ClientThread::init(const char* host, const char* port) {
  serv = new RemoteServer(*this);
  serv->init(host, port);
  serv->join();
  serv->move(-1);
  running = true;
}

void ClientThread::exit() {
  if (running) {
    running = false;
    serv->exit();
  }
}

void ClientThread::move(char dir) {
  if (player) {
    player->move(dir);
    const std::string state = pGameState->getState();
    update(state.data(), state.size());
  } else if (serv) {
    serv->move(dir);
  }
}

void ClientThread::update(const char* state, const uint32_t size) {
  view->update(state, size);
}

void ClientThread::initView(int id, int N) {
  this->id = id;
  view->init(id, N);

  if (pGameState && !player)
    this->player = pGameState->addPlayer(id);
}

void ClientThread::setState(GameState* gameState) {
  pGameState = gameState;

  if (pGameState)
    this->player = pGameState->getPlayer(id);
}
