#ifndef _PLAYERMANAGER_GUARD
#define _PLAYERMANAGER_GUARD

#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <string>

#include "ServerThread.h"
#include "ClientThread.h"
#include "GameState.h"

class PlayerManager
{
  public:
    PlayerManager(int sockfd, GameState& gameState, ServerThread& st, ClientThread& ct);
    ~PlayerManager();

    void start(int playerId = -1);
    void addMessage(const std::string& msg);

    void createBackupServer();
    void sendNewServer(const std::string& host, const std::string& port);

  private:
    int sockfd;
    std::mutex sock_mtx;
    std::atomic<bool> joined;
    std::atomic<int> id;

    GameState& gameState;
    ServerThread& st;
    ClientThread& ct;

    std::string buffer;

    std::thread msg_thread;
    std::mutex msg_mx;
    std::condition_variable cv;

    std::atomic<int> nb_msg;
    std::atomic<bool> running;

    void stop();
    void processMessage();
    void join();
    void move(const std::string& cmd);
    void sendState(uint32_t head, bool send_size = false);
    void sendHead(uint32_t head);
    void sendMsg(const std::string& msg);
    const std::string getHost() const;
};

#endif
