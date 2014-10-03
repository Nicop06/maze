#include "ClientViewNcurses.h"
#include "ClientThread.h"

#include <algorithm>
#include <iostream>
#include <arpa/inet.h>

ClientViewNcurses::ClientViewNcurses(ClientThread& clientThread)
  : ClientView(clientThread), N(0), win(NULL), running(false) {
  std::cout << "Waiting for game to start..." << std::endl;
}

ClientViewNcurses::~ClientViewNcurses() {
  if (running) {
    running = false;
    nodelay(stdscr, TRUE);
    loop_th.join();
  }

  endwin();
}

void ClientViewNcurses::init(int id, int N) {
  if (!running && N != 0) {
    this->N = N;
    this->id = id;

    initscr();
    cbreak();
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);

    win = newwin(N+2, 2*N+1, 2, 2);

    running = true;
    loop_th = std::thread(&ClientViewNcurses::loop, this);
  }
}

void ClientViewNcurses::loop() {
  int c;

  while ((c = getch()) != 'q' && running) {
    char dir = 0;

    switch (c) {
      case KEY_UP:
        dir = 'N';
        break;
      case KEY_DOWN:
        dir = 'S';
        break;
      case KEY_LEFT:
        dir = 'W';
        break;
      case KEY_RIGHT:
        dir = 'E';
        break;
    }

    if (dir != 0)
      clientThread.move(dir);
  }

  clientThread.exit();
}

int ClientViewNcurses::update(const char* state, const uint32_t size) {
  if (size < 8 || !win || !running)
    return -1;

  const int* data = (int*) state;
  const int* max_data;

  int T = ntohl(*data);
  int P = ntohl(*(data + 1));
  unsigned int exp_size = 8 * T + 16 * P + 8;

  data += 2;
  max_data = data + 2 * T;

  if (size < exp_size || (T+P) > N*N)
    return -1;

  int maxx, maxy, begx, begy;
  getmaxyx(win, maxy, maxx);
  getbegyx(win, begy, begx);

  clear();
  wclear(win);
  box(win, 0, 0);
  mvprintw(0, 0, "Presss 'q' to exit");

  for ( ; data < max_data; data += 2) {
    int x = ntohl(*data);
    int y = ntohl(*(data + 1));
    mvwaddch(win, y+1, 2*x+1, 'T');
  }

  int status_y = maxy + 1;
  data = max_data;
  max_data = data + 4 * P;

  int best_id(id), best_score(0);
  for ( ; data < max_data; data += 4) {
    int p_id = ntohl(*data);
    int p_T = ntohl(*(data + 1));
    int x = ntohl(*(data + 2));
    int y = ntohl(*(data + 3));

    if (p_T > best_score)
      best_id = p_id;

    if (p_id != id) {
      mvprintw(++status_y, begx+1, "Player %d: %d", p_id, p_T);
      mvwaddch(win, y+1, 2*x+1, 'P');
    } else {
      mvprintw(begy-1, begx+1, "Id: %d   Score: %d", p_id, p_T);
      mvwaddch(win, y+1, 2*x+1, 'Y');
    }
  }

  if (T == 0) {
    std::string msg;
    if (best_id == id) {
      msg = "You won";
    } else {
      msg = "Player " + std::to_string(best_id) + " won";
    }

    wclear(win);
    box(win, 0, 0);
    mvwprintw(win, (maxy - 1) / 2, std::max(1, (int)((maxx - msg.length()) / 2)), "%s", msg.data());
  }

  refresh();
  wrefresh(win);

  return exp_size;
}
