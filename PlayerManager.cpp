#include "PlayerManager.h"
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <algorithm>
#include <iostream>
#include "config.h"

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
  } else {
    throw std::string("Cannot create player");
  }
}

void PlayerManager::start(){
  if(running)
    msg_thread = std::thread(&PlayerManager::processMessage, this);
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

void PlayerManager::waitForJoin() {
  size_t pos;
  std::string cmd, msg;
  int head_init = htonl(INIT);

  if (!running)
    return;

  std::unique_lock<std::mutex> lck(msg_mx);
  while (nb_msg <= 0 && running) cv.wait(lck);
  pos = buffer.find('\0',0);
  cmd = buffer.substr(0,pos);
  buffer = buffer.substr(pos+1);
  lck.unlock();
  
  nb_msg--;

  if (cmd == "join") {
    int id = htonl(player->id());
    int N = htonl(gameState.getSize());
    msg.append((char*) &head_init, sizeof(int));
    msg.append((char*) &id, sizeof(int));
    msg.append((char*) &N, sizeof(int));
  }else{
     std::cerr << "Client " << player->id() << " did not join" << std::endl;
     stop();
     return;
  }
  
  if (send(sockfd, msg.data(), msg.size(), 0) <= 0) {
    std::cerr << "Error while sending N and id to client " << player->id() << std::endl;
    stop();
    return;
  }
  std::cerr << "Client " << player->id() << " joined" << std::endl;
}

void PlayerManager::processMessage() {
  size_t pos, old_pos;
  std::string tmp, cmd, msg;
  int head_state = htonl(STATE);
  
  msg.append((char*) &head_state, sizeof(int));
  msg += gameState.getState();
  if (send(sockfd, msg.data(), msg.size(), 0) <= 0) {
    std::cerr << "Error while sending the state for the first time to client " << player->id() << std::endl;
    stop();
    return;
  }

  while (running) {
    msg.clear();
    std::unique_lock<std::mutex> lck(msg_mx);
    while (nb_msg <= 0 && running) cv.wait(lck);
    tmp.swap(buffer);
    lck.unlock();

    if (!running)
      break;

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

      msg.append((char*) &head_state, sizeof(int));
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

bool PlayerManager::sendBackup(std::string port){
  std::string msg;
  int head_backup = htonl(BACKUP);
  int portSize = htonl(port.size());
  msg.append((char*) &head_backup, sizeof(int));
  msg.append((char*) &portSize, sizeof(int));
  msg += port;
  if (send(sockfd, msg.data(), msg.size(), 0) <= 0) {
    std::cerr << "Error while sending backup message to client " << player->id() << std::endl;
    stop();
    return false;
  }
  return true;
}

void PlayerManager::sendBackupIp(std::string ip, std::string port){
  std::string msg;
  int head_backIp = htonl(BACK_IP);
  int portSize = htonl(port.size());
  int ipSize = htonl(ip.size());
  msg.append((char*) &head_backIp, sizeof(int));
  msg.append((char*) &ipSize, sizeof(int));
  msg.append((char*) &portSize, sizeof(int));
  msg += ip;
  msg += port;
  if (send(sockfd, msg.data(), msg.size(), 0) <= 0) {
    std::cerr << "Error while sending backup ip to client " << player->id() << std::endl;
    stop();
  }
}

std::string PlayerManager::getIpAddr(){
  socklen_t len;
  struct sockaddr_storage addr;
  char ipstr[INET6_ADDRSTRLEN];
  //int p;

  len = sizeof addr;
  getpeername(sockfd, (struct sockaddr*)&addr, &len);

  // deal with both IPv4 and IPv6:
  if (addr.ss_family == AF_INET) {
    struct sockaddr_in *s = (struct sockaddr_in *)&addr;
    //p = ntohs(s->sin_port);
    inet_ntop(AF_INET, &s->sin_addr, ipstr, sizeof ipstr);
  } else { // AF_INET6
    struct sockaddr_in6 *s = (struct sockaddr_in6 *)&addr;
    //p = ntohs(s->sin6_port);
    inet_ntop(AF_INET6, &s->sin6_addr, ipstr, sizeof ipstr);
  }
  return std::string(ipstr);
}

int PlayerManager::getPlayerId() const{
  return player->id();
}
