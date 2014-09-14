CXX=g++
CXXFLAGS=-g3 -O0 -Wall -c -std=gnu++11 -pthread
LDFLAGS=-std=gnu++11 -pthread
EXEC=maze_server maze_client
SRC=$(wildcard *.cpp)
OBJ= $(SRC:.cpp=.o)

SERVER_SRC=mainServer.cpp ServerThread.cpp ServerThread.h
SERVER_OBJ=$(SERVER_SRC:.cpp=.o)

CLIENT_SRC=mainClient.cpp ClientThread.cpp ClientThread.h
CLIENT_OBJ=$(CLIENT_SRC:.cpp=.o)

all: $(EXEC)

maze_server: $(SERVER_OBJ)
	$(CXX) -o $@ $^ $(LDFLAGS)

maze_client: $(CLIENT_OBJ)
	$(CXX) -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	$(CXX) -o $@ $^ $(CXXFLAGS)

.PHONY: clean mrproper

clean:
	rm -rf *.o

mrproper: clean
	rm -rf $(EXEC)
