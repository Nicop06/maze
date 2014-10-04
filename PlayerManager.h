#ifndef _PLAYERMANAGER_GUARD
#define _PLAYERMANAGER_GUARD

#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <string>

#include "ServerThread.h"
#include "GameState.h"
#include "Player.h"

class PlayerManager
{
  public:
    PlayerManager(int sockfd, GameState& gameState, ServerThread& st);
    ~PlayerManager();

    void init(int playerId);
    void start();
    void stop();
    void addMessage(const std::string& msg);

    void createServer();
    void sendServer(const std::string& host, const std::string& port);

  private:
    int sockfd;
    bool joined;

    GameState& gameState;
    ServerThread& st;
    Player* player;

    std::string buffer;

    std::thread msg_thread;
    std::mutex msg_mx;
    std::condition_variable cv;

    std::atomic<int> nb_msg;
    std::atomic<bool> running;

    void processMessage();
    void join();
    void move(const std::string& cmd);
    void sendState(uint32_t head, bool send_size = false);
    void sendMsg(const std::string& msg);
};

#endif
