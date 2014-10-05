#include "ClientViewNcurses.h"
#include "ClientThread.h"

#include <algorithm>
#include <iostream>
#include <arpa/inet.h>

ClientViewNcurses::ClientViewNcurses(ClientThread& clientThread) : ClientView(clientThread),
  N(0), main_win(NULL), game_win(NULL), dbg_win(NULL), running(false) {
  std::cout << "Waiting for game to start..." << std::endl;
}

ClientViewNcurses::~ClientViewNcurses() {
  if (running) {
    running = false;
    nodelay(stdscr, TRUE);
  }

  if (loop_th.joinable())
    loop_th.join();

  if (stdscr)
    endwin();
}

void ClientViewNcurses::init(int id, int N) {
  if (!running && N != 0) {
    this->N = N;
    this->id = id;

    initscr();
    halfdelay(2);
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);

    int maxx, maxy;
    getmaxyx(stdscr, maxy, maxx);

    int width = std::max(2*N+6, 20);

    main_win = newwin(std::max(2*N+2, maxy), width, 0, 0);
    game_win = newwin(N+2, 2*N+1, 2, 2);
    dbg_win = newwin(std::max(N+10, maxy), maxx - width, 0, width);

    if (!main_win || !game_win || !dbg_win)
      return;

    this->setp(0, 0);
    this->setg(0, 0, 0);
    std::cout.rdbuf(this);
    std::cerr.rdbuf(this);

    scrollok(dbg_win, true);

    running = true;
    loop_th = std::thread(&ClientViewNcurses::loop, this);
  }
}

int ClientViewNcurses::overflow(int c) {
  int ret = c;
  if(c != EOF) {
    if(waddch(dbg_win, (chtype)c) == ERR)
      ret = EOF;
  }
  if((EOF==c) || std::isspace(c)) {
    if(EOF == this->sync())
      ret = EOF;
  }
  return ret;
}

int ClientViewNcurses::sync() {
  return (wrefresh(dbg_win) == ERR) ? EOF : 0;
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
      case ERR:
        dir = -1;
        break;
    }

    if (dir != 0)
      clientThread.move(dir);
  }

  clientThread.exit();
}

int ClientViewNcurses::update(const char* state, const size_t size) {
  if (size < 8 || !running)
    return -1;

  const int* data = (int*) state;
  const int* max_data;

  int T = ntohl(*data);
  int P = ntohl(*(data + 1));
  const size_t exp_size = 8 * T + 16 * P + 8;

  if (size < exp_size || (T+P) > N*N)
    return -1;

  data += 2;
  max_data = data + 2 * T;

  int maxx, maxy, begx, begy;
  getmaxyx(game_win, maxy, maxx);
  getbegyx(game_win, begy, begx);

  wclear(game_win);
  wclear(main_win);
  box(game_win, 0, 0);
  mvwprintw(main_win, 0, 0, "Presss 'q' to exit");

  for ( ; data < max_data; data += 2) {
    int x = ntohl(*data);
    int y = ntohl(*(data + 1));
    mvwaddch(game_win, y+1, 2*x+1, 'T');
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
      mvwprintw(main_win, ++status_y, begx+1, "Player %d: %d", p_id, p_T);
      mvwaddch(game_win, y+1, 2*x+1, 'P');
    } else {
      mvwprintw(main_win, begy-1, begx+1, "Id: %d   Score: %d", p_id, p_T);
      mvwaddch(game_win, y+1, 2*x+1, 'Y');
    }
  }

  if (T == 0) {
    std::string msg;
    if (best_id == id) {
      msg = "You won";
    } else {
      msg = "Player " + std::to_string(best_id) + " won";
    }

    wclear(game_win);
    box(game_win, 0, 0);
    mvwprintw(game_win, (maxy - 1) / 2, std::max(1, (int)((maxx - msg.length()) / 2)), "%s", msg.data());
  }

  refresh();
  wrefresh(main_win);
  wrefresh(game_win);

  return exp_size;
}
