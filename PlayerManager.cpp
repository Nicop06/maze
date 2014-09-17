#include "PlayerManager.h"
#include <sys/socket.h>
#include <unistd.h>
#include <algorithm>
#include <iostream>

PlayerManager::PlayerManager(int sockfd, GameState& gameState)
  : sockfd(sockfd), gameState(gameState), nb_msg(0) {
}

PlayerManager::~PlayerManager() {
  stop();
}

void PlayerManager::init() {
  running = true;
  player = gameState.addPlayer(sockfd);
  msg_thread = std::thread(&PlayerManager::processMessage, this);
}

void PlayerManager::stop() {
  std::cout << "Closing connection with client " << player->id() << std::endl;
  running = false;
  cv.notify_one();

  if (msg_thread.joinable())
    msg_thread.join();
  close(sockfd);
}

void PlayerManager::addMessage(std::string msg) {
  if (!running)
    return;

  msg_mx.lock();
  size_t nb_new = std::count(msg.begin(), msg.end(), '\0');
  buffer += msg;
  msg_mx.unlock();

  if (nb_new > 0)
    nb_msg += nb_new;

  cv.notify_one();
}

void PlayerManager::processMessage() {
  std::unique_lock<std::mutex> lock(msg_mx, std::defer_lock);
  size_t pos, old_pos;
  std::string tmp, cmd;

  while (running) {
    lock.lock();
    cv.wait(lock, [=]{ return nb_msg > 0 || running; });
    tmp.swap(buffer);
    lock.unlock();

    old_pos = 0;

    while ((pos = tmp.find('\0', old_pos)) != std::string::npos) {
      cmd += tmp.substr(old_pos, pos - old_pos);

      if (cmd == "exit") {
        stop();
        gameState.removePlayer(player->id());
        return;
      }

      if (cmd == "S" || cmd == "E" || cmd == "N" || cmd == "W")
          player->move(cmd[0]);

      std::string state = gameState.getState();
      if (send(sockfd, state.data(), state.size(), 0) == -1) {
        std::cerr <<"Error while sending the state to client " << player->id() << std::endl;
        stop();
        return;
      }

      nb_msg--;
      old_pos = pos + 1;
      cmd.erase();
    }

    cmd = tmp.substr(old_pos);
    tmp.erase();
  }
}

