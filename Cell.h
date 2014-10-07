#ifndef _CELL_GUARD
#define _CELL_GUARD

#include <string>
#include <atomic>

class Cell {
  public:
    Cell(int x, int y) : x(x), y(y) {}
    virtual ~Cell() {};

    virtual bool isTreasure() const = 0;
    virtual bool isPlayer() const = 0;

    // The position of the cell
    std::atomic<int> x, y;
};

#endif
