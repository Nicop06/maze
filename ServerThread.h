class ServerThread {
public:
	ServerThread();
	~ServerThread();
	void startConnection();
	static void* get_in_addr(struct sockaddr *sa);
private:
	static void echoing(int inSockFd);
};
