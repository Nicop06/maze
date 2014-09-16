#ifndef _CELL_GUARD
#define _CELL_GUARD

class Cell {
  public:
    Cell(int x, int y) : mx(x), my(y) {}
    virtual ~Cell() {};

    int x() const { return mx; }
    int y() const { return my; }

    virtual bool isTreasure() const = 0;
    virtual bool isPlayer() const = 0;

  protected:
    // The position of the cell
    int mx, my;
};

#endif
