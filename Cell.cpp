#include "Cell.h"
#include <arpa/inet.h>

std::string Cell::getState() {
  std::string state;

  int nx = htonl(mx);
  int ny = htonl(my);

  state.append((char*) &nx, 4);
  state.append((char*) &ny, 4);
  return state;
}
