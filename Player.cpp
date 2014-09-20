#include "Player.h"
#include "GameState.h"
#include <arpa/inet.h>

void Player::move(char dir) {
  new_x = mx;
  new_y = my;

  switch(dir) {
    case 'S':
      new_y++;
      break;

    case 'E':
      new_x++;
      break;

    case 'N':
      new_y--;
      break;

    case 'W':
      new_x--;
      break;

    default:
      break;
  }

  pGameState->updatePosition(this, new_x, new_y);
}

void Player::updatePosition() {
  if (new_x >= 0 && new_y >= 0) {
    mx = new_x;
    my = new_y;
    new_x = -1;
    new_y = -1;
  }
}

std::string Player::getState() {
  std::string state;

  int nid = htonl(mId);
  int nT = htonl(T);

  state.append((char*) &nid, 4);
  state.append((char*) &nT, 4);
  state += Cell::getState();

  return state;
}

