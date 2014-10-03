#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <cstring>

#include <string>
#include <iostream>
#include <thread>

#include "ClientThread.h"
#include "ServerThread.h"
#include "RemoteServer.h"
#include "ClientViewNcurses.h"

ClientThread::ClientThread() : st(NULL), id(-1), running(false) {
  view = new ClientViewNcurses(*this);
}

ClientThread::~ClientThread() {
  exit();
  delete view;
  delete serv;
  delete st;
}

void ClientThread::initClientServer(int N, int M, const char* port, const char* servPort){
  st = new ServerThread(N,M, this);
  std::thread acceptClient;

  st->init(port,servPort);
  acceptClient = std::thread(&ServerThread::acceptClients, st);
  init("localhost",port);

  acceptClient.join();//clients are now all accepted by the server
  serv->wait();
  std::cout << "End wait\n";
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
  serv->move(dir);
}

void ClientThread::update(const char* state, const uint32_t size) {
  view->update(state, size);
}

void ClientThread::initView(int id, int N) {
  this->id = id;
  view->init(id, N);
}
