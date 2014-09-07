#include <iostream>
#include <string>

#include "ServerThread.h"

using namespace std;

int main(int argc, char* argv[]) {
	ServerThread st = ServerThread();
	try{
		st.startConnection();
	} catch(string const& e){
		cerr << "Error : " << e << endl;
	}
	return 0;
}
