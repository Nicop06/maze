#include "PlayerManager.h"
#include "config.h"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <algorithm>
#include <iostream>
#include <cstring>

PlayerManager::PlayerManager(int sockfd, GameState& gameState, ServerThread& st, ClientThread& ct)
  : sockfd(sockfd), joined(false), id(-1), gameState(gameState), st(st), ct(ct), nb_msg(0), running(false) {
}

PlayerManager::~PlayerManager() {
  stop();

  if (msg_thread.joinable())
    msg_thread.join();

  close(sockfd);
}

void PlayerManager::start(int playerId) {
  this->joined = playerId == -1;
  this->id = playerId;
  bool created = playerId == -1 ? true : gameState.addPlayer(playerId);

  if (!running && created) {
    running = true;

    std::cout << "Connection with client " << playerId << std::endl;

    msg_thread = std::thread(&PlayerManager::processMessage, this);
  } else {
    throw std::string("Cannot create player");
  }
}

void PlayerManager::stop() {
  if (running) {
    running = false;
    cv.notify_one();

    std::cout << "Closing connection with client " << id << std::endl;
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
      cmd.append(tmp, old_pos, pos - old_pos);

      if (cmd == "exit") {
        stop();
        gameState.removePlayer(id);
        return;
      }

      if (id >= 0) {
        if (cmd == "join") {
          if (!joined) {
            join();
            joined = true;
          }
        } else if (cmd == "S" || cmd == "E" || cmd == "N" || cmd == "W" || cmd == "NoMove") {
          move(cmd);
        }
      } else if (cmd.compare(0, 7, "connect") == 0) {
        if (tmp.length() < old_pos + 12)
          break;

        if (tmp[old_pos + 11] == '\0') {
          int* p_id = (int*)(tmp.data() + old_pos + 7);
          this->id = ntohl(*p_id);
          pos = old_pos + 11;
          nb_msg -= std::count(tmp.begin() + old_pos, tmp.begin() + pos, '\0');
        }
      }

      if (cmd == "server") {
        if (nb_msg < 3)
          break;

        std::string host, port;
        old_pos = pos + 1;
        pos = tmp.find('\0', old_pos);
        host = tmp.substr(old_pos, pos - old_pos);

        old_pos = pos + 1;
        pos = tmp.find('\0', old_pos);
        port = tmp.substr(old_pos, pos - old_pos);

        st.newServer(this, host, port);
        nb_msg -= 2;
      } else if (cmd.compare(0, sizeof(MOVE_PLAYER) - 1, MOVE_PLAYER) == 0) {
        if (tmp.length() < old_pos + sizeof(MOVE_PLAYER) + 4)
          break;

        char dir = tmp[old_pos + sizeof(MOVE_PLAYER) - 1];
        int* p_id = (int*)(tmp.data() + old_pos + sizeof(MOVE_PLAYER));
        ct.movePlayer(ntohl(*p_id), dir);
        sendHead(PLAYER_MOVED);

        pos = old_pos + sizeof(MOVE_PLAYER) + 3;
        nb_msg -= std::count(tmp.begin() + old_pos, tmp.begin() + pos, '\0');

      } else if (cmd == REQUEST_STATE) {
        if (ct.releaseState()) {
          sendHead(STATE_ACQUIRED);
        } else {
          sendHead(NOT_OWNER);
        }
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
  uint32_t nId = htonl(id);
  uint32_t N = htonl(gameState.getSize());
  uint32_t size = htonl(8);
  uint32_t head = htonl(INIT_CLIENT);

  msg.append((char*) &head, 4);
  msg.append((char*) &size, 4);
  msg.append((char*) &nId, 4);
  msg.append((char*) &N, 4);

  sendMsg(msg);
}

void PlayerManager::move(const std::string& cmd) {
  if (cmd != "NoMove")
    ct.syncMove(id, cmd[0]);

  sendState(htonl(UPDATE_STATE));
}

void PlayerManager::createBackupServer() {
  sendState(htonl(CREATE_SERVER), true);
}

void PlayerManager::sendState(uint32_t head, bool send_size) {
  std::string msg;
  msg.append((char*) &head, 4);

  const std::string state = gameState.getState();
  uint32_t size = htonl(state.size() + (send_size ? 4 : 0));
  msg.append((char*) &size, 4);

  if (send_size) {
    uint32_t N = htonl(gameState.getSize());
    msg.append((char*) &N, 4);
  }

  msg += state;

  sendMsg(msg);
}

void PlayerManager::sendHead(uint32_t head) {
  std::string msg;
  uint32_t n_head = ntohl(head);
  msg.append((char*) &n_head, 4);
  sendMsg(msg);
}

void PlayerManager::sendNewServer(const std::string& host, const std::string& port) {
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

  std::lock_guard<std::mutex> sock_lck(sock_mtx);
  while ((n = send(sockfd, p, len, 0)) > 0) {
    p += n;
    len -= n;
  }

  if (n < 0) {
    std::cerr << "Error while sending message to client " << id << std::endl;
    stop();
  }
}
