#include "ClientThread.h"
using namespace std;

int main(int argc, char* argv[]) {
	ClientThread ct = ClientThread();
	ct.startConnection(argc, argv);
	return 0;
}
