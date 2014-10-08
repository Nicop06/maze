#include <poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <cstring>
#include <string>
#include <iostream>

#include "RemoteServer.h"
#include "ServerThread.h"

RemoteServer::~RemoteServer() {
  if (running) {
    running = false;
    sendMsg("exit");
  }

  if (loop_th.joinable() && loop_th.get_id() != std::this_thread::get_id())
    loop_th.join();

  close(sockfd);
}

void RemoteServer::init(const char* host, const char* port) {
  if (running)
    return;

  struct addrinfo hints, *result, *rp;

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  int s = getaddrinfo(host, port, &hints, &result);
  if (s != 0)
    throw std::string(gai_strerror(s));

  // Try to connect to the list of addresses returned by getaddrinfo()

  for (rp = result; rp != NULL; rp = rp->ai_next) {
    sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if (sockfd == -1)
      continue;

    if (connect(sockfd, rp->ai_addr, rp->ai_addrlen) != -1)
      break;

    close(sockfd);
  }

  if (rp == NULL)
    throw std::string("Couldn't connect to server.");

  freeaddrinfo(result);

  running = true;
  loop_th = std::thread(&RemoteServer::loop, this);
}

void RemoteServer::loop() {
  char buf[BUFSIZE];
  std::string buffer;
  uint32_t head;
  size_t size(0);
  ssize_t len;
  int* data;
  struct pollfd pfd;

  pfd.fd = sockfd;
  pfd.events = POLLIN | POLLHUP;

  while (running) {
    if (poll(&pfd, 1, 100) > 0) {
      if ((len = recv(sockfd, buf, BUFSIZE, MSG_DONTWAIT)) <= 0) {
        ct.delServer(this);
        return;
      }
      buffer.append(buf, len);
    }

    if (!running)
      return;

    if (size == 0 && buffer.size() >= 8) {
      data = (int*) buffer.data();
      head = ntohl(data[0]);
      size = ntohl(data[1]);
    }

    if (size > 0 && buffer.size() >= size + 8) {
      data = (int*) (buffer.data() + 8);
      switch (head) {
        case INIT_CLIENT:
          ct.initView(ntohl(data[0]), ntohl(data[1]));
          break;
        case UPDATE_STATE:
          ct.update(buffer.data() + 8, size);
          break;
        case CREATE_SERVER:
          if (!createServer(ntohl(data[0]), buffer.data() + 12, size - 4))
            ct.delServer(this);
          break;
        case NEW_SERVER:
          newServer(buffer.data() + 8, buffer.data() + buffer.find('\0', 8) + 1);
          break;
        case PLAYER_MOVED:
          ct.moveDone();
          break;
        case STATE_ACQUIRED:
          ct.stateAcquired(true);
          break;
        case NOT_OWNER:
          ct.stateAcquired(false);
          break;
      }

      buffer.erase(0, size + 8);
      size = 0;
    }
  }
}

bool RemoteServer::move(char dir) {
  std::string msg;

  switch (dir) {
    case 'N':
    case 'E':
    case 'W':
    case 'S':
      msg += dir;
      break;

    default:
      msg = "NoMove";
      break;
  }

  return sendMsg(msg);
}

bool RemoteServer::join() {
  return sendMsg("join");
}

bool RemoteServer::connectSrv(int id) {
  std::string msg = "connect";
  uint32_t nId = htonl(id);
  msg.append((char*) &nId, 4);
  return sendMsg(msg);
}

bool RemoteServer::createServer(int N, const char* state, size_t size) {
  const ServerThread* st = ct.startServer(N, state, size);
  std::string msg = "server";
  msg += '\0';
  msg += st ? getHost() : "";
  msg += '\0';
  msg += st ? st->getPort() : "";

  // Send the new backup server address and port to the main server
  return sendMsg(msg);
}

void RemoteServer::newServer(const char* host, const char* port) {
  // Connect to the new received server
  RemoteServer* serv = new RemoteServer(ct);
  if (serv) {
    try {
      serv->init(host, port);
      ct.addServer(serv);
    } catch (const std::string& e) {
      std::cerr << "Error: " << e << std::endl;
      delete serv;
    }
  }
}

bool RemoteServer::movePlayer(int id, char dir) {
  std::string msg = MOVE_PLAYER;
  msg += dir;
  uint32_t n_id = htonl(id);
  msg.append((char*) &n_id, 4);

  return sendMsg(msg);
}

bool RemoteServer::requestStateOwnership() {
  return sendMsg(REQUEST_STATE);
}

bool RemoteServer::sendMsg(const std::string& msg) {
  ssize_t n;
  size_t len = msg.size() + 1;
  const char* p = msg.data();

  std::lock_guard<std::mutex> sock_lck(sock_mtx);
  while ((n = send(sockfd, p, len, 0)) > 0) {
    p += n;
    len -= n;
  }

  if (n < 0)
    return false;

  return true;
}

const std::string RemoteServer::getHost() const {
  char host[INET6_ADDRSTRLEN];
  struct sockaddr_storage addr;
  socklen_t len = sizeof(addr);

  getpeername(sockfd, (struct sockaddr*)&addr, &len);

  // deal with both IPv4 and IPv6:
  if (addr.ss_family == AF_INET) {
    struct sockaddr_in *s = (struct sockaddr_in *) &addr;
    inet_ntop(AF_INET, &s->sin_addr, host, sizeof(host));
  } else { // AF_INET6
    struct sockaddr_in6 *s = (struct sockaddr_in6 *) &addr;
    inet_ntop(AF_INET6, &s->sin6_addr, host, sizeof(host));
  }

  return host;
}
