#ifndef _PLAYER_GUARD
#define _PLAYER_GUARD

#include "Cell.h"
#include <ostream>

class GameState;

class Player: public Cell {
  public:
    Player(int x, int y, int id, GameState* gameState)
      : Cell(x, y), mId(id), pGameState(gameState) {}
    ~Player() {}

    void incNbTreasures() { T++; }
    int id() const { return mId; }

    bool isTreasure() const { return false; }
    bool isPlayer() const { return true; }

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
