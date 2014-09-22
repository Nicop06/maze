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

    void init(const char* port = PORT);
    void acceptClients();
    void loop();
    void exit();

  private:
    int sockfd;
    std::vector<struct pollfd> fds;

    GameState gameState;
    std::map<int, PlayerManager*> pms;

    static bool running;
};

#endif
