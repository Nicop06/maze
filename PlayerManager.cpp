#include "PlayerManager.h"
#include "config.h"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <algorithm>
#include <iostream>

PlayerManager::PlayerManager(int sockfd, GameState& gameState)
  : sockfd(sockfd), joined(false), gameState(gameState), player(NULL), nb_msg(0), running(false) {
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
  uint32_t nb_new = std::count(msg.begin(), msg.end(), '\0');
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

      cmd.append(tmp, old_pos, pos - old_pos);

      if (cmd == "exit") {
        stop();
        if (player)
          gameState.removePlayer(player->id());
        return;
      }

      if (player) {
        if (cmd == "join" && !joined) {
          join();
          joined = true;
        } else if (cmd == "S" || cmd == "E" || cmd == "N" || cmd == "W" || cmd == "NoMove") {
          move(cmd);
        }
      } else if (joined && cmd.compare(0, 7, "connect") && tmp.length() > old_pos + 12 && tmp[old_pos + 12] == '\0') {
        int* id = (int*)(tmp.data() + old_pos + 7);
        player = gameState.getPlayer(ntohl(*id));
        pos = old_pos + 12;
        nb_msg -= std::count(tmp.begin() + old_pos, tmp.begin() + pos, '\0');
      }

      nb_msg--;
      old_pos = pos + 1;
      cmd.erase();
    }

    cmd = tmp.substr(old_pos);
    tmp.erase();
  }
}

void PlayerManager::join() {
  int id = htonl(player->id());
  int N = htonl(gameState.getSize());
  uint32_t size = htonl(8);
  uint32_t head = htonl(INIT_CLIENT);

  msg.append((char*) &head, 4);
  msg.append((char*) &size, 4);
  msg.append((char*) &id, 4);
  msg.append((char*) &N, 4);

  sendMsg();
}

void PlayerManager::move(const std::string& cmd) {
  if (cmd != "NoMove")
    player->move(cmd[0]);

  uint32_t head = htonl(UPDATE_STATE);
  msg.append((char*) &head, 4);

  const std::string state = gameState.getState();
  uint32_t size = htonl(state.size());
  msg.append((char*) &size, 4);
  msg += state;

  sendMsg();
}

void PlayerManager::sendMsg() {
  if (send(sockfd, msg.data(), msg.size(), 0) <= 0) {
    std::cerr << "Error while sending the state to client " << player->id() << std::endl;
    stop();
  }

  msg.clear();
}
