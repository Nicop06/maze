#include "Player.h"
#include <arpa/inet.h>

void Player::move(char dir, GameState::callback synchronize) {
  int new_x(mx), new_y(my);

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
      return;
  }

  pGameState->updatePosition(this, new_x, new_y, synchronize);
}

std::string Player::getState() {
  std::string state;

  uint32_t nid = htonl(mId);
  uint32_t nT = htonl(T);

  state.append((char*) &nid, 4);
  state.append((char*) &nT, 4);
  state += Cell::getState();

  return state;
}

