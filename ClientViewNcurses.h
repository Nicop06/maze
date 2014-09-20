#ifndef _CLIENTVIEWNCURSES_GUARD
#define _CLIENTVIEWNCURSES_GUARD

#include "ClientView.h"

#include <ncurses.h>

class ClientViewNcurses : public ClientView {
  public:
    ClientViewNcurses(int id, ClientThread& clientThread);
    ~ClientViewNcurses();
    
    void init(int N);

    // Update the view
    bool update(std::string state);

  private:
    // The size of the game
    int N;

    // The windows
    WINDOW* win;
};

#endif
