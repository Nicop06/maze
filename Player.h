#ifndef _PLAYER_GUARD
#define _PLAYER_GUARD

#include "Cell.h"
#include <ostream>

class GameState;

class Player: public Cell {
  public:
    Player(int x, int y, int id, GameState* gameState)
      : Cell(x, y), mId(id), pGameState(gameState), T(0) {}
    ~Player() {}

    int nbTreasures()
    {
      return T;
    }

    void incNbTreasures()
    {
      T++;
    }

    int id() {
      return mId;
    }

    bool isTreasure()
    {
      return false;
    }

    bool isPlayer()
    {
      return true;
    }

    void move(char dir);

    friend std::ostream& operator<<(std::ostream& stream, const Player& player);

  private:
    // The id of the player
    int mId;

    // The gameState
    GameState* pGameState;

    // The number of treasures
    int T;
};

std::ostream& operator<<(std::ostream& stream, const Player& player);

#endif
