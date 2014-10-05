#include <iostream>
#include <string>
#include <thread>

#include "ServerThread.h"
#include "ClientThread.h"
#include "RemoteServer.h"
#include "config.h"

void usage() {
  std::cout << "Usage: maze -s N M [PORT] or HOST [PORT]" << std::endl;
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
      if (argc < 4 || argc > 5)
        usage();

      ServerThread *st = new ServerThread(std::stoi(argv[2]), std::stoi(argv[3]), ct);
      const char* port = argc == 5 ? argv[4] : PORT;
      st->init(port);
      st->acceptClients();
      ct.init(st);
    } else {
      if(argc > 3)
        usage();

      RemoteServer* serv = new RemoteServer(ct);
      const char* host = argv[1];
      const char* port = argc == 3 ? argv[2] : PORT;
      serv->init(host, port);
      ct.init(serv);
    }

    ct.loop();
  } catch(std::string const& e) {
    std::cerr << "Error : " << e << std::endl;
    return -1;
  }

  return 0;
}
