#include "ClientViewNcurses.h"

#include <arpa/inet.h>

ClientViewNcurses::ClientViewNcurses(int id, ClientThread& clientThread)
  : ClientView(id, clientThread), win(NULL) {
  initscr();
  cbreak();
  noecho();
  curs_set(0);
  keypad(stdscr, TRUE);
}
 
ClientViewNcurses::~ClientViewNcurses() {
  endwin();
}

void ClientViewNcurses::init(int N) {
  if (this->N != N) {
    this->N = N;
    if (win) {
      wclear(win);
      delwin(win);
    }

    refresh();
    win = newwin(N+2, 2*N+2, 2, 2);
    box(win, 0, 0);
    wrefresh(win);
  }
}

bool ClientViewNcurses::update(std::string state) {
  if (state.size() < 8 || !win)
    return false;

  const int* data = (int*) state.data();
  const int* max_data;

  int T = ntohl(*data);
  int P = ntohl(*(data + 1));

  data += 2;
  max_data = data + 2 * T;

  //if (state.size() < (unsigned)(8 * T + 4 * P + 8))
    //return false;

  int maxx, maxy, begx, begy;
  getmaxyx(win, maxy, maxx);
  getbegyx(win, begy, begx);

  wclear(win);
  box(win, 0, 0);

  for ( ; data < max_data; data += 2) {
    int x = ntohl(*data);
    int y = ntohl(*(data + 1));
    mvwaddch(win, y+1, 2*x+1, 'T');
  }

  int status_y = maxy + 1;
  data = max_data;
  max_data = data + 4 * P;

  for ( ; data < max_data; data += 4) {
    int p_id = ntohl(*data);
    int p_T = ntohl(*(data + 1));
    int x = ntohl(*(data + 2));
    int y = ntohl(*(data + 3));

    if (p_id != id) {
      mvprintw(++status_y, begx+1, "Player %d: %d", p_id, p_T);
      mvwaddch(win, y+1, 2*x+1, 'P');
    } else {
      mvprintw(begy-1, begx+1, "Id: %d   Score: %d", p_id, p_T);
      mvwaddch(win, y+1, 2*x+1, 'Y');
    }
  }  

  refresh();
  wrefresh(win);

  return true;
}
