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
}

void ClientThread::initClientServer(int N, int M, const char* port, const char* servPort){
  st = new ServerThread(N,M, this);
  std::thread acceptClient, serverLoop;

  st->init(port,servPort);
  acceptClient = std::thread(&ServerThread::acceptClients, st);
  init("localhost",port);

  acceptClient.join();//clients are now all accepted by the server
  serverLoop = std::thread(&ServerThread::loop, st);
  serv->wait();
  serverLoop.join();
}

void ClientThread::initClient(const char* host, const char* port){
  init(host, port);
  serv->wait();
}

void ClientThread::init(const char* host, const char* port) {
  serv = new RemoteServer(*this);
  serv->init(host, port);
  serv->join();
}

void ClientThread::exit() {
  if (running)
    running = false;
}

void ClientThread::move(char dir) {
  serv->move(dir);
}

void ClientThread::update(const char* state, const size_t size) {
  view->update(state, size);
}

void ClientThread::initView(int id, int N) {
  this->id = id;
  view->init(id, N);
}
