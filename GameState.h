#ifndef _GAMESTATE_GUARD
#define _GAMESTATE_GUARD

#include <atomic>
#include <mutex>
#include <condition_variable>
#include <unordered_set>
#include <map>
#include <functional>

class Player;
class Cell;
class Treasure;

class GameState {
  public:
    typedef std::function<void(void)> callback;

    GameState(int N, int M);
    ~GameState();

    void initState(const char* state, size_t size);
    void initTreasures();

    bool addPlayer(int id);
    void removePlayer(int id);

    void move(int id, char dir, callback synchronize = NULL);

    std::string getState();
    int getSize() const;
    int getNbPlayers() const;

  private:
    std::mutex state_mutex;

    // The size of the grid
    std::atomic<int> N;

    // The total number of treasures
    std::atomic<int> M;

    // The number of available treasures
    std::atomic<int> T;

    // The number of player
    std::atomic<int> P;

    // The grid
    Cell*** grid;

    // The treasures
    std::unordered_set<Treasure*> treasures;

    // The players
    std::map<int, Player*> players;

    inline bool checkBounds(int x, int y) const;
};

#endif
