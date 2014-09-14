#ifndef _CELL_GUARD
#define _CELL_GUARD

class Cell
{
  public:
    Cell(int x, int y) : mx(x), my(y) {}
    virtual ~Cell() {};

    int x()
    {
      return mx;
    }

    int y()
    {
      return my;
    }

    virtual bool isTreasure() = 0;
    virtual bool isPlayer() = 0;

  protected:
    // The position of the cell
    int mx, my;
};

#endif
