#ifndef _PLAYER_GUARD
#define _PLAYER_GUARD

#include "Cell.h"
#include <ostream>

class GameState;

class Player: public Cell {
  public:
    Player(int x, int y, int id, GameState* gameState)
      : Cell(x, y), mId(id), pGameState(gameState), new_x(-1), new_y(-1) {}
    ~Player() {}

    void incNbTreasures() { T++; }
    int id() const { return mId; }

    bool isTreasure() const { return false; }
    bool isPlayer() const { return true; }

    void updatePosition();

    void move(char dir);

    std::string getState();

  private:
    // The id of the player
    int mId;

    // The gameState
    GameState* pGameState;

    // The number of treasures
    int T;

    // Temporary variables for position update
    int new_x, new_y;
};

#endif
