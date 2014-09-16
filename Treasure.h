#ifndef _TREASURE_GUARD
#define _TREASURE_GUARD

#include "Cell.h"
#include <ostream>
#include <arpa/inet.h>

class Treasure: public Cell
{
  public:
    Treasure(int x, int y) : Cell(x, y) {}
    ~Treasure() {}

    bool isTreasure()
    {
      return true;
    }

    bool isPlayer()
    {
      return false;
    }

    friend std::ostream& operator<<(std::ostream& stream, const Treasure& treasure);
};

std::ostream& operator<<(std::ostream& stream, const Treasure& treasure);

#endif
