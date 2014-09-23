#include "PlayerManager.h"
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <algorithm>
#include <iostream>

PlayerManager::PlayerManager(int sockfd, GameState& gameState)
  : sockfd(sockfd), gameState(gameState), nb_msg(0) {
}

PlayerManager::~PlayerManager() {
  stop();

  if (msg_thread.joinable())
    msg_thread.join();

  close(sockfd);
}

void PlayerManager::init(int playerId) {
  player = gameState.addPlayer(playerId);

  if (player) {
    running = true;
    std::cout << "Connection with client " << player->id() << std::endl;
    msg_thread = std::thread(&PlayerManager::processMessage, this);
  } else {
    throw std::string("Cannot create player");
  }
}

void PlayerManager::stop() {
  if (running) {
    running = false;
    cv.notify_one();
    std::cout << "Closing connection with client " << player->id() << std::endl;
  }
}

void PlayerManager::addMessage(const std::string& msg) {
  if (!running)
    return;

  std::unique_lock<std::mutex> lck(msg_mx);
  size_t nb_new = std::count(msg.begin(), msg.end(), '\0');
  buffer += msg;

  if (nb_new > 0)
    nb_msg += nb_new;

  cv.notify_one();
}

void PlayerManager::processMessage() {
  size_t pos, old_pos;
  std::string tmp, cmd;

  while (running) {
    std::unique_lock<std::mutex> lck(msg_mx);
    while (nb_msg <= 0 && running) cv.wait(lck);
    tmp.swap(buffer);
    lck.unlock();

    if (!running)
      break;

    old_pos = 0;

    while ((pos = tmp.find('\0', old_pos)) != std::string::npos) {
      std::string msg;
      cmd += tmp.substr(old_pos, pos - old_pos);

      if (cmd == "exit") {
        stop();
        gameState.removePlayer(player->id());
        return;
      }

      if (cmd == "join") {
        int id = htonl(player->id());
        int N = htonl(gameState.getSize());
        msg.append((char*) &id, 4);
        msg.append((char*) &N, 4);
      }

      if (cmd == "S" || cmd == "E" || cmd == "N" || cmd == "W")
          player->move(cmd[0]);

      msg += gameState.getState();

      if (send(sockfd, msg.data(), msg.size(), 0) <= 0) {
        std::cerr << "Error while sending the state to client " << player->id() << std::endl;
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
