#include "GameState.h"
#include <random>
#include <chrono>

GameState::GameState(int N, int M) : N(N), M(M), T(0), treasures(M)
{
  grid = new Cell**[N];
  for (int i = 0; i < N; i++) {
    grid[i] = new Cell*[N];
  }

  if (M > N * N)
    return;

  unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
  std::minstd_rand generator(seed);
  std::uniform_int_distribution<int> distribution(0, N-1);

  while (T < M) {
    int x = distribution(generator);
    int y = distribution(generator);

    if (!grid[x][y]) {
      Treasure* treasure = new Treasure(x, y);
      grid[x][y] = treasure;
      treasures.insert(treasure);
      T++;
    }
  }
}

GameState::~GameState()
{
  for (Treasure* treasure: treasures)
    delete treasure;

  treasures.clear();

  for (const auto& pair: players) {
    Player* player = pair.second;
    delete player;
  }

  players.clear();

  for (int i = 0; i < N; i++)
    delete[] grid[i];

  delete[] grid;
}

bool GameState::updatePosition(Player* player, int new_x, int new_y)
{
  if (checkBounds(new_x, new_y)) {
    Cell* cell = grid[new_x][new_y];

    if (!cell || cell->isTreasure()) {
      if (cell && cell->isTreasure()) {
        player->incNbTreasures();
        T--;
        treasures.erase(dynamic_cast<Treasure*>(cell));
        delete cell;
      }

      grid[player->x()][player->y()] = NULL;
      grid[new_x][new_y] = player;
      return true;
    }
  }

  return false;
}

Player* GameState::addPlayer(int id)
{
  if (T + P >= N * N)
    return NULL;

  unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
  std::minstd_rand generator(seed);
  std::uniform_int_distribution<int> distribution(0, N-1);

  while (true) {
    int x = distribution(generator);
    int y = distribution(generator);

    if (!grid[x][y]) {
      Player* player = new Player(x, y, id, this);
      grid[x][y] = player;
      players.insert(std::make_pair(id, player));
      P++;
      return player;
    }
  }
}

void GameState::removePlayer(int id)
{
  Player* player = players[id];

  int x(player->x());
  int y(player->y());

  if (grid[x][y] == player)
    grid[x][y] = NULL;

  delete player;
}

void GameState::print()
{
  for (int y = 0; y < N; y++) {
    for (int x = 0; x < N; x++) {
      Cell* cell = grid[x][y];
      if (cell) {
        if (cell->isTreasure()) {
          std::cout << "T";
        } else {
          std::cout << "P";
        }
      } else {
        std::cout << "*";
      }
    }
    std::cout << std::endl;
  }
}

std::ostream& operator<<(std::ostream& stream, const GameState& gameState)
{
  stream << htonl(gameState.T) << htonl(gameState.P);

  for (Treasure* treasure: gameState.treasures)
    stream << treasure;

  for (const auto& pair: gameState.players) {
    Player* player = pair.second;
    stream << player;
  }

  return stream;
}
