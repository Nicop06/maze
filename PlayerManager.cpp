#include "PlayerManager.h"
#include "config.h"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <algorithm>
#include <iostream>
#include <cstring>

PlayerManager::PlayerManager(int sockfd, GameState& gameState, ServerThread& st)
  : sockfd(sockfd), joined(false), gameState(gameState), st(st), player(NULL), nb_msg(0), running(false) {
}

PlayerManager::~PlayerManager() {
  stop();

  if (msg_thread.joinable())
    msg_thread.join();

  close(sockfd);
}

void PlayerManager::init(int playerId) {
  joined = playerId == -1;

  if (playerId >= 0)
    player = gameState.addPlayer(playerId);

  if (!running && (player || playerId == -1)) {
    running = true;

    if (player) {
      std::cout << "Connection with client " << player->id() << std::endl;
    } else {
      std::cout << "Connection with existing client" << std::endl;
    }

    msg_thread = std::thread(&PlayerManager::processMessage, this);
  } else {
    throw std::string("Cannot create player");
  }
}

void PlayerManager::stop() {
  if (running) {
    running = false;
    cv.notify_one();

    if (player)
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

      if (cmd == "server") {
        if (nb_msg < 3)
          continue;

        std::string host, port;
        old_pos = pos + 1;
        pos = tmp.find('\0', old_pos);
        host = tmp.substr(old_pos, pos - old_pos);

        old_pos = pos + 1;
        pos = tmp.find('\0', old_pos);
        port = tmp.substr(old_pos, pos - old_pos);

        st.newServer(this, host, port);
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
  std::string msg;
  int id = htonl(player->id());
  int N = htonl(gameState.getSize());
  uint32_t size = htonl(8);
  uint32_t head = htonl(INIT_CLIENT);

  msg.append((char*) &head, 4);
  msg.append((char*) &size, 4);
  msg.append((char*) &id, 4);
  msg.append((char*) &N, 4);

  sendMsg(msg);
}

void PlayerManager::move(const std::string& cmd) {
  std::string msg;
  if (cmd != "NoMove")
    st.syncMove(player, cmd[0]);

  sendState(htonl(UPDATE_STATE));
}

void PlayerManager::createServer() {
  sendState(htonl(CREATE_SERVER), true);
}

void PlayerManager::sendState(uint32_t head, bool send_size) {
  std::string msg;
  msg.append((char*) &head, 4);

  const std::string state = gameState.getState();
  uint32_t size = htonl(state.size() + (send_size ? 4 : 0));
  msg.append((char*) &size, 4);

  if (send_size) {
    int N = htonl(gameState.getSize());
    msg.append((char*) &N, 4);
  }

  msg += state;

  sendMsg(msg);
}

void PlayerManager::sendServer(const std::string& host, const std::string& port) {
  std::string msg;
  uint32_t head = htonl(NEW_SERVER);
  uint32_t size = htonl(host.size() + port.size() + 2);
  msg.append((char*) &head, 4);
  msg.append((char*) &size, 4);
  msg += host;
  msg += '\0';
  msg += port;
  msg += '\0';

  sendMsg(msg);
}

void PlayerManager::sendMsg(const std::string& msg) {
  ssize_t n;
  size_t len = msg.size();
  const char* p = msg.data();

  while ((n = send(sockfd, p, len, 0)) > 0) {
    p += n;
    len -= n;
  }

  if (n < 0) {
    std::cerr << "Error while sending message to client " << player ? player->id() : "" << std::endl;
    stop();
  }
}
