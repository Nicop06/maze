#include <iostream>
#include <string>

#include "ClientThread.h"

void usage(const char* name) {
  std::cout << "Usage: " << name << " HOST [PORT]" << std::endl;
}

int main(int argc, const char* argv[]) {
	ClientThread ct;

	try {
    if (argc == 2) {
      ct.init(argv[1]);
    } else if (argc == 3) {
      ct.init(argv[1], argv[2]);
    } else {
      usage(argv[0]);
      return -1;
    }
	} catch(std::string const& e){
    std::cerr << "Error : " << e << std::endl;
    usage(argv[0]);
    return -1;
	}

  ct.loop();

	return 0;
}
