#include "GameState.h"
#include <arpa/inet.h>
#include <random>
#include <chrono>

GameState::GameState(int N, int M) : N(N), M(M), T(0), P(0), treasures(M) {
  grid = new Cell**[N];
  for (int i = 0; i < N; ++i) {
    grid[i] = new Cell*[N];
  }

}

GameState::~GameState() {
  std::lock_guard<std::mutex> lck(state_mutex);
  for (Treasure* treasure: treasures)
    delete treasure;

  treasures.clear();

  for (const auto& pair: players) {
    Player* player = pair.second;
    delete player;
  }

  players.clear();

  for (int i = 0; i < N; ++i)
    delete[] grid[i];

  delete[] grid;
}

void GameState::initTreasures() {
  std::lock_guard<std::mutex> lck(state_mutex);
  unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
  std::minstd_rand generator(seed);
  std::uniform_int_distribution<int> distribution(0, N-1);

  if (M > N * N)
    throw std::string("Cannot create treasures");

  while (T < M) {
    int x = distribution(generator);
    int y = distribution(generator);

    if (!grid[x][y]) {
      Treasure* treasure = new Treasure(x, y);
      grid[x][y] = treasure;
      treasures.insert(treasure);
      ++T;
    }
  }
}

void GameState::updatePosition(Player* player, int new_x, int new_y) {
  std::lock_guard<std::mutex> lck(state_mutex);

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
      player->updatePosition();
    }
  }
}

bool GameState::checkBounds(int x, int y) const {
  return x >= 0 && y >= 0 && x < N && y < N;
}

Player* GameState::addPlayer(int id) {
  std::lock_guard<std::mutex> lck(state_mutex);
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
      players[id] = player;
      ++P;
      return player;
    }
  }
}

void GameState::removePlayer(int id) {
  std::lock_guard<std::mutex> lck(state_mutex);
  Player* player = players[id];

  if (player) {
    int x(player->x());
    int y(player->y());

    if (grid[x][y] == player)
      grid[x][y] = NULL;

    players.erase(player->id());
    delete player;
  }
}

std::string GameState::getState() {
  std::lock_guard<std::mutex> lck(state_mutex);
  std::string state;

  int nT = htonl(T);
  int nP = htonl(P);

  state.append((char*) &nT, 4);
  state.append((char*) &nP, 4);

  for (Treasure* treasure: treasures)
    state += treasure->getState();

  for (const auto& pair: players) {
    Player* player = pair.second;
    state += player->getState();
  }

  return state;
}

