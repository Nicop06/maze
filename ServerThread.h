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

    void init(const char* port = PORT, const char* servPort = SERV_PORT);
    void acceptClients();

    void wait() { if (loop_th.joinable()) loop_th.join(); }

  private:
    int sockfd;
    int otherSockfd; //socket to the other server
    int clientId;
    std::string port;
    std::string servPort;
    std::vector<struct pollfd> fds;

    GameState gameState;
    ClientThread& ct;
    std::map<int, PlayerManager*> pms; //sockfd to playerManager

    std::thread loop_th;
    std::atomic<bool> running;

    void loop();

    void chooseBackup();
};

#endif
