#include <iostream>
#include <string>
#include <thread>

#include "ServerThread.h"
#include "ClientThread.h"

int main(int argc, char* argv[]) {
  if(std::string("-s").compare(argv[1])==0 && argc>=4 && argc<=6){
    //this is the main server, we have to launch server and client
    ClientThread ct;
    try {
      if (argc == 4) {
        ct.initClientServer(std::stoi(argv[2]), std::stoi(argv[3]));
      } else if (argc == 5) {
        ct.initClientServer(std::stoi(argv[2]), std::stoi(argv[3]),argv[4]);
      } else{
        ct.initClientServer(std::stoi(argv[2]), std::stoi(argv[3]),argv[4], argv[5]);
      }
    } catch(std::string const& e) {
      std::cerr << "Error : " << e << std::endl;
    }
  }else if(argc>=2 && argc <=3){
    //this is a single client
    ClientThread ct;
    try {
      if (argc == 2) {
        ct.initClient(argv[1]);
      } else if (argc == 3) {
        ct.initClient(argv[1], argv[2]);
      }
    } catch(std::string const& e){
      std::cerr << "Error : " << e << std::endl;
    }
  }else{
    std::cout << "Usage: " << argv[0] << "-s N M [PORT] [SERV_PORT] or HOST [PORT]" << std::endl;
    std::cout << "  N: grid size." << std::endl;
    std::cout << "  M: number of treasures." << std::endl;
  }
  return 0;
}
