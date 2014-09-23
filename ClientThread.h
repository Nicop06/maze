#ifndef _CLIENTTHREAD_GUARD
#define _CLIENTTHREAD_GUARD

#include "config.h"
#include "ClientView.h"

#include <poll.h>

#include <thread>
#include <atomic>

class ClientThread {
  public:
    ClientThread();
    ~ClientThread();

    void init(const char* host, const char* port = PORT);
    void exit();
    void loop();
    void move(char dir);

  private:
    ClientView* view;

    int sockfd;
    int otherSockfd; //socket to the backup server
    struct pollfd pfd;

    std::string buffer;
    char buf[BUFSIZE];

    std::atomic<bool> running;

    void read();
};

#endif
