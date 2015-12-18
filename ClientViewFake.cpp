#include "ClientViewFake.h"
#include "ClientThread.h"

#include <random>
#include <chrono>

ClientViewFake::ClientViewFake(ClientThread& clientThread) : ClientView(clientThread), running(false) {
}

ClientViewFake::~ClientViewFake() {
  if (loop_th.joinable())
    loop_th.join();
}

void ClientViewFake::init(int id, int N) {
  if (!running && N != 0) {
    this->N = N;
    this->id = id;

    running = true;
    loop_th = std::thread(&ClientViewFake::loop, this);
  }
}

void ClientViewFake::loop() {
  int c;

  unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
  std::minstd_rand generator(seed);
  std::uniform_int_distribution<int> distribution(0, 3);

  while (running) {
    char dir = 0;
    c = distribution(generator);

    switch (c) {
      case 0:
        dir = 'N';
        break;
      case 1:
        dir = 'S';
        break;
      case 2:
        dir = 'W';
        break;
      case 3:
        dir = 'E';
        break;
    }

    if (dir != 0)
      clientThread.move(dir);

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  clientThread.stop();
}

int ClientViewFake::update(const char*, const size_t size) {
  return size;
}
