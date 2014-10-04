#include <iostream>
#include <string>
#include <thread>

#include "ServerThread.h"
#include "ClientThread.h"

void usage() {
  std::cout << "Usage: maze -s N M [PORT] [SERV_PORT] or HOST [PORT]" << std::endl;
  std::cout << "  N: grid size." << std::endl;
  std::cout << "  M: number of treasures." << std::endl;
  exit(-1);
}

int main(int argc, char* argv[]) {
  if (argc < 2)
    usage();

  ClientThread ct;
  try {
    if(std::string(argv[1]) == "-s") {
      if (argc == 4) {
        ct.initClientServer(std::stoi(argv[2]), std::stoi(argv[3]));
      } else if (argc == 5) {
        ct.initClientServer(std::stoi(argv[2]), std::stoi(argv[3]), argv[4]);
      } else {
        usage();
      }
    } else if(argc>=2 && argc <=3) {
      if (argc == 2) {
        ct.initClient(argv[1]);
      } else if (argc == 3) {
        ct.initClient(argv[1], argv[2]);
      }
    }
  } catch(std::string const& e){
    std::cerr << "Error : " << e << std::endl;
    return -1;
  }

  return 0;
}
