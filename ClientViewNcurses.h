#ifndef _CLIENTVIEWNCURSES_GUARD
#define _CLIENTVIEWNCURSES_GUARD

#include "ClientView.h"

#include <thread>
#include <atomic>
#include <ncurses.h>

class ClientViewNcurses : public ClientView {
  public:
    ClientViewNcurses(ClientThread& clientThread);
    ~ClientViewNcurses();

    void init(int id, int N);

    // Update the view
    int update(const std::string& state);

  private:
    // The size of the game
    int N;

    // The windows
    WINDOW* win;

    // The main thread
    std::thread loop_th;
    std::atomic<bool> running;

    // The main loop
    void loop();
};

#endif
