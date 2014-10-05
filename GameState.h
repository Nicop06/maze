#ifndef _GAMESTATE_GUARD
#define _GAMESTATE_GUARD

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

    Player* getPlayer(int id) const;
    Player* addPlayer(int id);
    void removePlayer(int id);

    void updatePosition(Player* player, int new_x, int new_y, callback synchronize);
    inline bool checkBounds(int x, int y) const;

    void synchronize();

    std::string getState();
    int getSize() const { return N; }
    int getNbPlayers() const { return P; }

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
