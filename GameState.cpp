#include "GameState.h"
#include "Player.h"
#include "Cell.h"
#include "Treasure.h"
#include "config.h"

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
  for (const Treasure* treasure: treasures)
    delete treasure;

  treasures.clear();

  for (const auto& pair: players)
    delete pair.second;

  players.clear();

  for (int i = 0; i < N; ++i)
    delete[] grid[i];

  delete[] grid;
}

void GameState::initState(const char* state, size_t size) {
  std::lock_guard<std::mutex> lck(state_mutex);
  if (size < 8)
    return;

  const int* data = (int*) state;
  const int* max_data;

  T = ntohl(*data);
  M = ntohl(*data);
  P = ntohl(*(data + 1));

  const size_t exp_size = 8 * T + 16 * P + 8;

  if (size < exp_size || (T+P) > N*N)
    return;

  data += 2;
  max_data = data + 2 * T;

  for ( ; data < max_data; data += 2) {
    int x = ntohl(*data);
    int y = ntohl(*(data + 1));

    Treasure *t = new Treasure(x, y);
    treasures.insert(t);
    grid[x][y] = t;
  }

  data = max_data;
  max_data = data + 4 * P;

  for ( ; data < max_data; data += 4) {
    int p_id = ntohl(*data);
    int p_T = ntohl(*(data + 1));
    int x = ntohl(*(data + 2));
    int y = ntohl(*(data + 3));

    players[p_id] = new Player(x, y, p_id, p_T);
    grid[x][y] = players[p_id];
  }
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

void GameState::move(int id, char dir) {
  std::unique_lock<std::mutex> lck(state_mutex);
  auto it = players.find(id);
  if (it == players.end())
    return;

  Player *player = it->second;
  int new_x(player->x), new_y(player->y);

  switch(dir) {
    case 'S':
      new_y++;
      break;

    case 'E':
      new_x++;
      break;

    case 'N':
      new_y--;
      break;

    case 'W':
      new_x--;
      break;

    default:
      return;
  }

  if (checkBounds(new_x, new_y)) {
    Cell* cell = grid[new_x][new_y];

    if (!cell || cell->isTreasure()) {
      if (cell && cell->isTreasure()) {
        player->T++;
        T--;
        treasures.erase(dynamic_cast<Treasure*>(cell));
        delete cell;
      }

      grid[player->x][player->y] = NULL;
      grid[new_x][new_y] = player;
      player->x = new_x;
      player->y = new_y;
    }
  }
}

bool GameState::checkBounds(int x, int y) const {
  return x >= 0 && y >= 0 && x < N && y < N;
}

bool GameState::addPlayer(int id) {
  std::lock_guard<std::mutex> lck(state_mutex);

  auto it = players.find(id);

  if (T + P >= N * N || it != players.end())
    return false;

  unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
  std::minstd_rand generator(seed);
  std::uniform_int_distribution<int> distribution(0, N-1);

  while (true) {
    int x = distribution(generator);
    int y = distribution(generator);

    if (!grid[x][y]) {
      Player* player = new Player(x, y, id);
      grid[x][y] = player;
      players[id] = player;
      ++P;
      return true;
    }
  }
}

void GameState::removePlayer(int id) {
  std::lock_guard<std::mutex> lck(state_mutex);
  auto it = players.find(id);

  if (it != players.end()) {
    Player *player = it->second;
    int x(player->x);
    int y(player->y);

    if (grid[x][y] == player)
      grid[x][y] = NULL;

    players.erase(it);
    delete player;
    --P;
  }
}

std::string GameState::getState() {
  std::lock_guard<std::mutex> lck(state_mutex);
  std::string state;

  uint32_t nT = htonl(T);
  uint32_t nP = htonl(P);

  state.append((char*) &nT, 4);
  state.append((char*) &nP, 4);

  for (Treasure* treasure: treasures) {
    uint32_t nx = htonl(treasure->x);
    uint32_t ny = htonl(treasure->y);

    state.append((char*) &nx, 4);
    state.append((char*) &ny, 4);
  }

  for (const auto& pair: players) {
    Player *p = pair.second;
    uint32_t nId = htonl(p->id);
    uint32_t nT = htonl(p->T);
    uint32_t nx = htonl(p->x);
    uint32_t ny = htonl(p->y);

    state.append((char*) &nId, 4);
    state.append((char*) &nT, 4);
    state.append((char*) &nx, 4);
    state.append((char*) &ny, 4);
  }

  return state;
}

int GameState::getSize() const {
  return N;
}

int GameState::getNbPlayers() const {
  return P;
}
