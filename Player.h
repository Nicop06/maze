#ifndef _PLAYER_GUARD
#define _PLAYER_GUARD

#include "Cell.h"
#include "GameState.h"

class Player: public Cell {
  public:
    Player(GameState* state, int x, int y, int id, int T)
      : Cell(x, y), mId(id), T(0), pGameState(gameState) {}
    Player(GameState* state, int x, int y, int id) : Player(state, x, y, id, 0) {}
    ~Player() {}

    void incNbTreasures() { T++; }
    int id() const { return mId; }

    bool isTreasure() const { return false; }
    bool isPlayer() const { return true; }

    void move(char dir, bool async = false);

    std::string getState();

    friend void GameState::updatePosition(Player* player, int new_x, int new_y);

  private:
    // The id of the player
    int mId;

    // The number of treasures
    int T;

    // The gameState
    GameState* pGameState;
};

#endif
