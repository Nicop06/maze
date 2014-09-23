#include <iostream>
#include <string>
#include <thread>

#include "ServerThread.h"
#include "ClientThread.h"

int main(int argc, char* argv[]) {
  if(std::string("-s").compare(argv[1])==0 && argc>=4 && argc<=6){
    //this is the main server, we have to launch server and client
    ServerThread st(std::stoi(argv[2]), std::stoi(argv[3]));
    ClientThread ct;
    std::thread acceptClient, clientLoop, serverLoop;
    try {
      if (argc == 4) {
        st.init();
        acceptClient = std::thread(&ServerThread::acceptClients, &st);
        ct.init("localhost");
      } else if (argc == 5) {
        st.init(argv[4]);
        acceptClient = std::thread(&ServerThread::acceptClients, &st);
        ct.init("localhost",argv[4]);
      } else{
        st.init(argv[4]);//st.init(argv[4],argv[5]);
        acceptClient = std::thread(&ServerThread::acceptClients, &st);
        ct.init("localhost",argv[4]);
      }
      clientLoop = std::thread(&ClientThread::loop, &ct);
      acceptClient.join();
      serverLoop = std::thread(&ServerThread::loop, &st);
      clientLoop.join();
      serverLoop.join();
    } catch(std::string const& e) {
      std::cerr << "Error : " << e << std::endl;
    }
  }else if(argc>=1 && argc <=2){
    //this is a single client
    ClientThread ct;
    try {
      if (argc == 2) {
        ct.init(argv[1]);
      } else if (argc == 3) {
        ct.init(argv[1], argv[2]);
      }
    } catch(std::string const& e){
      std::cerr << "Error : " << e << std::endl;
    }
    ct.loop();
  }else{
    std::cout << "Usage: " << argv[0] << "-s N M [PORT] [SERV_PORT] or HOST [PORT]" << std::endl;
    std::cout << "  N: grid size." << std::endl;
    std::cout << "  M: number of treasures." << std::endl;
  }
  return 0;
}
