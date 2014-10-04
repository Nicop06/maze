#ifndef _SERVERTHREAD_GUARD
#define _SERVERTHREAD_GUARD

#include "GameState.h"
#include "ClientThread.h"
#include "PlayerManager.h"
#include "config.h"

#include <atomic>
#include <thread>
#include <vector>
#include <map>
#include <poll.h>

class ServerThread {
  public:
    ServerThread(int N, int M, ClientThread& client);
    ~ServerThread();

    void init(const char* port = NULL);
    void acceptClients();

    void wait() { if (loop_th.joinable()) loop_th.join(); }

  private:
    std::string port;
    int sockfd;
    int clientId;
    std::vector<struct pollfd> fds;

    GameState gameState;
    ClientThread& ct;
    std::map<int, PlayerManager*> pms; //sockfd to playerManager

    std::thread loop_th;
    std::atomic<bool> running;

    void loop();
    bool tryBind(const char* port);
};

#endif
