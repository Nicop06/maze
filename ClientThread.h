class ClientThread {
public:
	ClientThread();
	~ClientThread();
	void startConnection(int argc, char* argv[]);
	static void* get_in_addr(struct sockaddr *sa);
};
