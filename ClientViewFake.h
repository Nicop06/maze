#ifndef _CLIENTVIEWFAKE_GUARD
#define _CLIENTVIEWFAKE_GUARD

#include "ClientView.h"

#include <thread>
#include <atomic>
#include <ncurses.h>
#include <iostream>

class ClientViewFake : public ClientView {
  public:
    ClientViewFake(ClientThread& clientThread);
    ~ClientViewFake();

    void init(int id, int N);

    // Update the view
    int update(const char* state, size_t size);

  private:
    // The size of the game
    int N;

    // The main thread
    std::thread loop_th;
    std::atomic<bool> running;

    // The main loop
    void loop();
};

#endif
