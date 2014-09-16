#include "Treasure.h"

std::ostream& operator<<(std::ostream& stream, const Treasure& treasure) {
  stream << htonl(treasure.mx) << htonl(treasure.my);
  return stream;
}
