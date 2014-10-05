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
    const ServerThread* startServer(int N, const char* state, size_t size);

    // Actions
    void move(char dir);
    void movePlayer(int id, char dir);
    void syncMove(Player* player, char dir);
    void sendSync(int id, char dir);
    void moveDone();

    void update(const char* state, size_t size);
    void initView(int id, int N);
    void setState(GameState* gameState);

  private:
    ClientView* view;
    ServerThread* st;
    std::unordered_set<RemoteServer*> servers;
    std::unordered_set<RemoteServer*> old_servers;
    std::mutex servers_mtx;

    int id;
    std::atomic<bool> initialized;

    GameState* pGameState;
    Player* player;

    std::atomic<bool> running;
    std::condition_variable cv_loop;
    std::mutex loop_mtx;

    std::atomic<int> nb_sync;
    std::condition_variable cv_sync;
    std::mutex sync_mtx;

    std::mutex update_mtx;

    void init();
    void delSrv(RemoteServer* srv);
    void cleanServers();
    void createBackups();
};

#endif
