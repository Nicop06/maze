#ifndef _GAMESTATE_GUARD
#define _GAMESTATE_GUARD

#include <mutex>
#include <condition_variable>
#include <unordered_set>
#include <map>

class Player;
class Cell;
class Treasure;

class GameState {
  public:
    GameState(int N, int M);
    ~GameState();

    void initTreasures();

    Player* getPlayer(int id) const;
    Player* addPlayer(int id);
    void removePlayer(int id);

    void updatePosition(Player* player, int new_x, int new_y, bool async);
    inline bool checkBounds(int x, int y) const;

    inline void synchronize();

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

    // This is for async call
    std::condition_variable cv_sync;
    std::mutex cv_lock;

    // The grid
    Cell*** grid;

    // The treasures
    std::unordered_set<Treasure*> treasures;

    // The players
    std::map<int, Player*> players;
};

#endif
