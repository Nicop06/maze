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

void GameState::initState(const char* state, uint32_t size) {
  std::lock_guard<std::mutex> lck(state_mutex);
  if (size < 8)
    return;

  const int* data = (int*) state;
  const int* max_data;

  T = ntohl(*data);
  P = ntohl(*(data + 1));

  const uint32_t exp_size = 8 * T + 16 * P + 8;

  if (size < exp_size || (T+P) > N*N)
    return;

  data += 2;
  max_data = data + 2 * T;

  for ( ; data < max_data; data += 2) {
    int x = ntohl(*data);
    int y = ntohl(*(data + 1));
    treasures.insert(new Treasure(x, y));
  }

  data = max_data;
  max_data = data + 4 * P;

  for ( ; data < max_data; data += 4) {
    int p_id = ntohl(*data);
    int p_T = ntohl(*(data + 1));
    int x = ntohl(*(data + 2));
    int y = ntohl(*(data + 3));
    players[p_id] = new Player(this, x, y, p_id, p_T);
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

void GameState::updatePosition(Player* player, int new_x, int new_y, GameState::async_callback async) {
  std::lock_guard<std::mutex> lck(state_mutex);

  if (async) {
    std::unique_lock<std::mutex> cv_lck(sync_mutex);
    async(*this);
    if (cv_sync.wait_for(cv_lck, std::chrono::seconds(LOCK_TIMEOUT)) == std::cv_status::timeout)
      return;
  }

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
      player->mx = new_x;
      player->my = new_y;
    }
  }
}

void GameState::synchronize() {
  cv_sync.notify_one();
}

bool GameState::checkBounds(int x, int y) const {
  return x >= 0 && y >= 0 && x < N && y < N;
}

Player* GameState::getPlayer(int id) const {
  auto it = players.find(id);
  return it == players.end() ? NULL : it->second;
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
      Player* player = new Player(this, x, y, id);
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
    --P;
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

