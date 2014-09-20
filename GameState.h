#ifndef _GAMESTATE_GUARD
#define _GAMESTATE_GUARD

#include "Cell.h"
#include "Player.h"
#include "Treasure.h"

#include <mutex>
#include <unordered_set>
#include <map>
#include <iostream>
#include <arpa/inet.h>

class GameState {
  public:
    GameState(int N, int M);
    ~GameState();

    void initTreasures();

    Player* addPlayer(int id);
    void removePlayer(int id);

    void updatePosition(Player* player, int new_x, int new_y);
    inline bool checkBounds(int x, int y) const;

    std::string getState();
    int getSize() const { return N; }

  private:
    std::mutex state_mutex;

    // The size of the grid
    int N;

    // The total number of treasures
    int M;

    // The number of available treasures
    int T;

    // The number of player
    int P;

    // The grid
    Cell*** grid;

    // The treasures
    std::unordered_set<Treasure*> treasures;

    // The players
    std::map<int, Player*> players;
};

#endif
