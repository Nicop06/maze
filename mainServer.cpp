#include "ServerThread.h"
using namespace std;

int main(int argc, char* argv[]) {
	ServerThread st = ServerThread();
	st.startConnection();
	return 0;
}
