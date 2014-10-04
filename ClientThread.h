#ifndef _CLIENTTHREAD_GUARD
#define _CLIENTTHREAD_GUARD

#include "config.h"
#include "ClientView.h"

#include <poll.h>
#include <condition_variable>
#include <atomic>
#include <mutex>
#include <unordered_set>

class ServerThread;
class RemoteServer;
class GameState;
class Player;

class ClientThread {
  public:
    ClientThread();
    ~ClientThread();
    void init(ServerThread* st);
    void init(RemoteServer* serv);
    void exit();
    void loop();

    void addServer(RemoteServer* serv, bool join = false);
    void delServer(RemoteServer* serv);

    // Actions
    void move(char dir);
    void update(const char* state, uint32_t size);
    void initView(int id, int N);
    void setState(GameState* gameState);

  private:
    ClientView* view;
    ServerThread* st;
    std::unordered_set<RemoteServer*> servers;

    int id;
    bool initialized;

    GameState* pGameState;
    Player* player;

    std::atomic<bool> running;
    std::condition_variable cv;
    std::mutex cv_mtx;

    void init();
};

#endif
