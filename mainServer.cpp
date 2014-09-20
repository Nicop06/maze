#include <iostream>
#include <string>

#include "ServerThread.h"

int main(int argc, char* argv[]) {
  if (argc < 3 || argc > 4) {
    std::cout << "Usage: " << argv[0] << " N M [PORT]" << std::endl;
    std::cout << "  N: grid size." << std::endl;
    std::cout << "  M: number of treasures." << std::endl;
    return -1;
  }

	ServerThread st(std::stoi(argv[1]), std::stoi(argv[2]));

	try {
    if (argc == 3) {
      st.init();
    } else if (argc == 4) {
      st.init(argv[3]);
    }

		st.acceptClients();
    st.loop();
	} catch(std::string const& e) {
    std::cerr << "Error : " << e << std::endl;
	}

	return 0;
}
