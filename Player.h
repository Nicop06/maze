#ifndef _PLAYER_GUARD
#define _PLAYER_GUARD

#include "Cell.h"
#include "GameState.h"

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

    std::string getState();

    friend void GameState::updatePosition(Player* player, int new_x, int new_y);

  private:
    // The id of the player
    int mId;

    // The gameState
    GameState* pGameState;

    // The number of treasures
    int T;
};

#endif
