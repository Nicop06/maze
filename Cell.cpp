#include "Cell.h"
#include <arpa/inet.h>

std::string Cell::getState() {
  std::string state;
  state.append(htonl(mx), 4);
  state.append(htonl(my), 4);
  return state;
}
