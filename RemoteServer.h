#ifndef _REMOTESERVER_GUARD
#define _REMOTESERVER_GUARD

#include "ClientThread.h"
#include "config.h"

#include <thread>
#include <atomic>

class RemoteServer {
  public:
    RemoteServer(ClientThread& ct) : ct(ct), running(false) {}
    ~RemoteServer();

    void init(const char* host, const char* port = PORT);
    void exit();

    // Actions
    bool move(char dir);
    bool join();
    bool connectSrv(int id);
    bool movePlayer(int id, char dir);

  private:
    ClientThread& ct;

    std::thread loop_th;

    std::atomic<bool> running;
    int sockfd;

    void loop();
    void createServer(int N, const char* state, uint32_t size);
    bool sendServer(const std::string& host);
    bool sendMsg(const std::string& msg);
    bool playerMoved();
    const std::string getHost() const;
};

#endif
