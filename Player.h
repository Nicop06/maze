#ifndef _PLAYER_GUARD
#define _PLAYER_GUARD

#include "Cell.h"

#include <atomic>

class Player: public Cell {
  public:
    Player(int x, int y, int id, int T) : Cell(x, y), id(id), T(T) {}
    Player(int x, int y, int id) : Player(x, y, id, 0) {}
    ~Player() {}

    bool isTreasure() const { return false; }
    bool isPlayer() const { return true; }

    // The id of the player
    std::atomic<int> id;

    // The number of treasures
    std::atomic<int> T;
};

#endif
