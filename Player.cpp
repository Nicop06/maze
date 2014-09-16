#include "Player.h"
#include "GameState.h"
#include <arpa/inet.h>

void Player::move(char dir)
{
  int new_x = mx, new_y = my;

  pGameState->lock();

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

  if (pGameState->updatePosition(this, new_x, new_y)) {
    mx = new_x;
    my = new_y;
  }

  pGameState->unlock();
}

std::ostream& operator<<(std::ostream& stream, const Player& player)
{
  stream << htonl(player.mId) << htonl(player.mx) << htonl(player.my) << htonl(player.T);
  return stream;
}
