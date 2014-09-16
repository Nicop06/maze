#ifndef _GAMESTATE_GUARD
#define _GAMESTATE_GUARD

#include "Cell.h"
#include "Player.h"
#include "Treasure.h"

#include <mutex>
#include <unordered_set>
#include <unordered_map>
#include <ostream>
#include <iostream>

class GameState
{
  public:
    GameState(int N, int M);
    ~GameState();

    void lock()
    {
      state_mutex.lock();
    }

    void unlock()
    {
      state_mutex.unlock();
    }

    Player* addPlayer(int id);
    void removePlayer(int id);

    bool updatePosition(Player* player, int new_x, int new_y);

    bool checkBounds(int x, int y)
    {
      return x >= 0 && y >= 0 && x < N && y < N;
    }

    void print();

    friend std::ostream& operator<<(std::ostream& stream, const GameState& gameState);

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
    std::unordered_map<int, Player*> players;
};

std::ostream& operator<<(std::ostream& stream, const GameState& gameState);

#endif
