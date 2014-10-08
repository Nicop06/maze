#ifndef _REMOTESERVER_GUARD
#define _REMOTESERVER_GUARD

#include "ClientThread.h"
#include "config.h"

#include <thread>
#include <atomic>
#include <mutex>

class RemoteServer {
  public:
    RemoteServer(ClientThread& ct) : ct(ct), running(false) {}
    ~RemoteServer();

    void init(const char* host, const char* port = PORT);

    // Actions
    bool move(char dir);
    bool join();
    bool connectSrv(int id);
    bool movePlayer(int id, char dir);
    bool requestStateOwnership();

  private:
    ClientThread& ct;

    std::thread loop_th;

    std::atomic<bool> running;
    int sockfd;
    std::mutex sock_mtx;

    void loop();
    bool createServer(int N, const char* state, size_t size);
    void newServer(const char* host, const char* port);
    bool sendMsg(const std::string& msg);
    const std::string getHost() const;
};

#endif
