#ifndef _PLAYERMANAGER_GUARD
#define _PLAYERMANAGER_GUARD

#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <string>

#include "GameState.h"
#include "Player.h"

class PlayerManager
{
  public:
    PlayerManager(int sockfd, GameState& gameState);
    ~PlayerManager();

    void init(int playerId);
    void start();
    void stop();
    void addMessage(const std::string& msg);

  private:
    int sockfd;
    bool joined;
    GameState& gameState;
    Player* player;
    std::string buffer;
    std::string msg;

    std::thread msg_thread;
    std::mutex msg_mx;
    std::condition_variable cv;

    std::atomic<int> nb_msg;
    std::atomic<bool> running;

    void processMessage();
    void join();
    void move(const std::string& cmd);
    void sendMsg();
};

#endif
