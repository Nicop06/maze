#ifndef _CLIENTVIEW_GUARD
#define _CLIENTVIEW_GUARD

#include <string>
#include <ncurses.h>

class ClientThread;

class ClientView {
  public:
    ClientView(ClientThread& clientThread) : id(0), clientThread(clientThread) {}
    virtual ~ClientView() {}

    virtual void init(int id, int N) = 0;

    // Update the view
    virtual int update(const char* state, uint32_t size) = 0;

  protected:
    // The id of the client
    int id;

    // The game thread
    ClientThread& clientThread;
};

#endif
