#include <iostream>
#include <string>

#include "ClientThread.h"

using namespace std;

int main(int argc, char* argv[]) {
	ClientThread ct = ClientThread();
	try{
		ct.startConnection(argc, argv);
	}catch(string const& e){
		cerr << "Error : " << e << endl;
	}
	return 0;
}
