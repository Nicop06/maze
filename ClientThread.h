#ifndef _CLIENTTHREAD_GUARD
#define _CLIENTTHREAD_GUARD

#include "config.h"
#include "ClientView.h"

#include <poll.h>

#include <thread>
#include <atomic>

class ServerThread;

class ClientThread {
  public:
    ClientThread();
    ~ClientThread();
    void initClientServer(int N, int M, const char* port = PORT, const char* servPort = SERV_PORT);
    void initClient(const char* host, const char* port = PORT);
    void exit();
    void move(char dir);
    int getId() const;

  private:
    ClientView* view;
    ServerThread* st;
    int id;
    std::string mPort;
    std::string mHost;
    int sockfd;
    int otherSockfd; //socket to the backup server
    struct pollfd pfd;
    
    std::string buffer;
    char buf[BUFSIZE];

    std::atomic<bool> running;

    void init(const char* host, const char* port = PORT);
    void loop();
    void read();
};

#endif
