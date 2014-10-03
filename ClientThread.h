#ifndef _CLIENTTHREAD_GUARD
#define _CLIENTTHREAD_GUARD

#include "config.h"
#include "ClientView.h"

#include <poll.h>

#include <thread>
#include <atomic>

class ServerThread;
class RemoteServer;

class ClientThread {
  public:
    ClientThread();
    ~ClientThread();
    void initClientServer(int N, int M, const char* port = PORT, const char* servPort = SERV_PORT);
    void initClient(const char* host, const char* port = PORT);
    void exit();
    int getId() const { return id; }

    // Actions
    void move(char dir);
    void update(const char* state, size_t size);
    void initView(int id, int N);

  private:
    ClientView* view;
    ServerThread* st;
    RemoteServer* serv;
    int id;

    std::atomic<bool> running;

    void init(const char* host, const char* port = PORT);
    void loop();
};

#endif
