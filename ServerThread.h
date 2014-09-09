#include <vector>
#include <thread>
#include <mutex>

class ServerThread {
public:
	ServerThread();
	~ServerThread();
	void initConnection();
	void acceptConnections();
	static void* get_in_addr(struct sockaddr *sa);
private:
	int sockfd;
	std::vector<int> inSockFds;
	std::mutex vect_mutex;
	void acceptInSock();
	void echoing(int inSockFd);
};
