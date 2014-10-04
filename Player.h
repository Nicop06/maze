#ifndef _PLAYER_GUARD
#define _PLAYER_GUARD

#include "Cell.h"
#include "GameState.h"

class Player: public Cell {
  public:
    Player(GameState* gameState, int x, int y, int id, int T)
      : Cell(x, y), mId(id), T(T), pGameState(gameState) {}
    Player(GameState* gameState, int x, int y, int id) : Player(gameState, x, y, id, 0) {}
    ~Player() {}

    void incNbTreasures() { T++; }
    int id() const { return mId; }

    bool isTreasure() const { return false; }
    bool isPlayer() const { return true; }

    void move(char dir, GameState::async_callback async = NULL);

    std::string getState();

    friend void GameState::updatePosition(Player* player, int new_x, int new_y, GameState::async_callback async);

  private:
    // The id of the player
    int mId;

    // The number of treasures
    int T;

    // The gameState
    GameState* pGameState;
};

#endif
