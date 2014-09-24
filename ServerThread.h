#ifndef _SERVERTHREAD_GUARD
#define _SERVERTHREAD_GUARD

#include "GameState.h"
#include "PlayerManager.h"
#include "config.h"
#include <vector>
#include <map>
#include <poll.h>

class ServerThread {
  public:
    ServerThread(int N, int M);
    ~ServerThread();

    void init(const char* port = PORT, const char* servPort = SERV_PORT);
    void acceptClients();
    void loop();
    void exit();

  private:
    int sockfd;
    int otherSockfd; //socket to the other server
    std::string port;
    std::string servPort;
    std::vector<struct pollfd> fds;

    GameState gameState;
    std::map<int, PlayerManager*> pms; //sockfd to playerManager
    static bool running;

    void waitClientsJoin();
    void chooseBackup();
};

#endif
