#ifndef _CLIENTTHREAD_GUARD
#define _CLIENTTHREAD_GUARD

#include "config.h"
#include "ClientView.h"

#include <poll.h>

#include <thread>
#include <atomic>

class ServerThread;
class RemoteServer;
class GameState;
class Player;

class ClientThread {
  public:
    ClientThread();
    ~ClientThread();
    void initClientServer(int N, int M, const char* port = PORT, const char* servPort = SERV_PORT);
    void initClient(const char* host, const char* port = PORT);
    void exit();

    // Actions
    void move(char dir);
    void update(const char* state, uint32_t size);
    void initView(int id, int N);
    void setState(GameState* gameState);

  private:
    ClientView* view;
    ServerThread* st;
    RemoteServer* serv;

    int id;
    bool initialized;

    GameState* pGameState;
    Player* player;

    std::atomic<bool> running;

    void init(const char* host, const char* port = PORT);
    void loop();
};

#endif
