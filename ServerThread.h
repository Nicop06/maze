#ifndef _SERVERTHREAD_GUARD
#define _SERVERTHREAD_GUARD

#include "GameState.h"
#include "ClientThread.h"
#include "config.h"

#include <mutex>
#include <atomic>
#include <thread>
#include <vector>
#include <map>
#include <poll.h>

class PlayerManager;

class ServerThread {
  public:
    ServerThread(int N, int M, ClientThread& client);
    ~ServerThread();

    void init(const char* port = NULL);
    void acceptClients();
    void connectClients();

    bool createBackupServer();
    void newServer(const PlayerManager* pm, const std::string& host, const std::string& port);
    void syncMove(int id, char dir);

    const std::string& getPort() const { return port; }

  private:
    std::string port;
    int sockfd;
    std::vector<struct pollfd> fds;

    GameState gameState;
    ClientThread& ct;
    std::map<int, PlayerManager*> pms; //sockfd to playerManager
    std::mutex pms_mtx;

    std::thread connect_th;
    std::thread loop_th;
    std::atomic<bool> running;

    std::atomic<bool> new_srv_created;
    std::condition_variable cv_new_srv;
    std::mutex new_srv_mtx;

    void loop();
    bool tryBind(const char* port);
    void acceptClient(int id);
    void connectClientsLoop();
};

#endif
