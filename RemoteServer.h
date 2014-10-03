#ifndef _REMOTESERVER_GUARD
#define _REMOTESERVER_GUARD

#include "ClientThread.h"
#include "config.h"

#include <thread>
#include <atomic>

class RemoteServer {
  public:
    RemoteServer(ClientThread& ct) : ct(ct), running(false) {}
    void init(const char* host, const char* port = PORT);
    void exit();

    // Actions
    void move(char dir);
    void join();

    void wait() { if(loop_th.joinable()) loop_th.join(); }

  private:
    ClientThread& ct;

    std::thread loop_th;

    std::atomic<bool> running;
    int sockfd;

    void loop();
};

#endif
