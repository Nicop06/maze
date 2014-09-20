#ifndef _CLIENTVIEW_GUARD
#define _CLIENTVIEW_GUARD

//#include "GameThread.h"

class ClientThread {
  public:
    void move(char dir) { m_dir = dir; }
  private:
    char m_dir;
};

#include <string>
#include <ncurses.h>

class ClientView {
  public:
    ClientView(int id, ClientThread& clientThread) : id(id), clientThread(clientThread) {}
    virtual ~ClientView() {}

    virtual void init(int N) = 0;

    // Update the view
    virtual bool update(std::string state) = 0;

  protected:
    // The id of the client
    int id;

    // The game thread
    ClientThread& clientThread;

    // Send move command to the game thread
    void move(int dir) { clientThread.move(dir); }
};

#endif
