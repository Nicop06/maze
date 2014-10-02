#ifndef _TREASURE_GUARD
#define _TREASURE_GUARD

#include "Cell.h"

class Treasure: public Cell {
  public:
    Treasure(int x, int y) : Cell(x, y) {}
    ~Treasure() {}

    bool isTreasure() const { return true; }
    bool isPlayer() const { return false; }
};

#endif
