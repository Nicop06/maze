#ifndef _CLIENTTHREAD_GUARD
#define _CLIENTTHREAD_GUARD

#include "config.h"
#include "ClientView.h"
#include "read_write_mutex.h"

#include <poll.h>
#include <condition_variable>
#include <atomic>
#include <mutex>
#include <set>

class ServerThread;
class RemoteServer;
class GameState;

class ClientThread {
  public:
    ClientThread(bool fake);
    ~ClientThread();
    void init(ServerThread* st);
    void init(RemoteServer* serv);
    void stop();
    void loop();

    void addServer(RemoteServer* serv, bool join = false);
    void delServer(RemoteServer* serv);
    const ServerThread* startServer(int N, const char* state, size_t size);
    void stopServer();

    // Actions
    void move(char dir);
    void movePlayer(int id, char dir);

    // Sync actions
    bool releaseState();
    void stateAcquired(bool acquired);
    void syncMove(int id, char dir);
    void moveDone();

    void update(const char* state, size_t size);
    void initView(int id, int N);
    void setState(GameState* gameState);

  private:
    ClientView* view;
    ServerThread* st;
    std::set<RemoteServer*> servers;
    std::mutex servers_mtx;
    read_write_mutex st_mtx;

    std::atomic<int> id;
    std::atomic<bool> initialized;

    GameState* pGameState;

    std::atomic<bool> running;
    std::condition_variable cv_loop;
    std::mutex loop_mtx;

    std::atomic<int> nb_sync;
    std::condition_variable cv_sync;
    std::mutex sync_mtx;
    std::mutex sync_move_mtx;

    std::atomic<bool> state_owner;
    std::atomic<int> nb_owner;
    std::mutex state_mtx;
    std::condition_variable cv_state;

    void _delServer(RemoteServer* srv);
    void createBackups();
    void sendSyncMove(int id, char dir);
};

#endif
