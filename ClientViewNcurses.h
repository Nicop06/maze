#ifndef _CLIENTVIEWNCURSES_GUARD
#define _CLIENTVIEWNCURSES_GUARD

#include "ClientView.h"

#include <thread>
#include <atomic>
#include <ncurses.h>
#include <iostream>

class ClientViewNcurses : public ClientView, public std::streambuf {
  public:
    ClientViewNcurses(ClientThread& clientThread);
    ~ClientViewNcurses();

    void init(int id, int N);

    virtual int overflow( int c );
    virtual int sync();

    // Update the view
    int update(const char* state, uint32_t size);

  private:
    // The size of the game
    int N;

    // The windows
    WINDOW *main_win, *game_win, *dbg_win;

    // The main thread
    std::thread loop_th;
    std::atomic<bool> running;

    // The main loop
    void loop();
};

#endif
