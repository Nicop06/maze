#include "Cell.h"
#include <arpa/inet.h>

std::string Cell::getState() {
  std::string state;

  uint32_t nx = htonl(mx);
  uint32_t ny = htonl(my);

  state.append((char*) &nx, 4);
  state.append((char*) &ny, 4);
  return state;
}
