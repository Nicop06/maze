#include <iostream>
#include <string>

#include "ServerThread.h"

using namespace std;

int main(int argc, char* argv[]) {
	ServerThread st;
	try{
		st.initConnection();
		st.acceptConnections();
	} catch(string const& e){
		cerr << "Error : " << e << endl;
	}
	return 0;
}
