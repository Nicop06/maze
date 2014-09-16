CXX=g++
CXXFLAGS=-g3 -O0 -Wall -c -std=gnu++11 -pthread
LDFLAGS=-std=gnu++11 -pthread
EXEC=maze_server maze_client
SRC=$(wildcard *.cpp)
OBJ= $(SRC:.cpp=.o)
INC=$(wildcard *.h)

SERVER_SRC=mainServer.cpp ServerThread.cpp GameState.cpp Player.cpp Treasure.cpp
SERVER_OBJ=$(SERVER_SRC:.cpp=.o)

CLIENT_SRC=mainClient.cpp ClientThread.cpp
CLIENT_OBJ=$(CLIENT_SRC:.cpp=.o)

all: $(EXEC)

maze_server: $(SERVER_OBJ)
	$(CXX) -o $@ $^ $(LDFLAGS)

maze_client: $(CLIENT_OBJ)
	$(CXX) -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	$(CXX) -o $@ $< $(CXXFLAGS)

# Headers dependencies
GameState.o: GameState.h Cell.h Player.h Treasure.h
Player.o: GameState.h Cell.h Player.h Treasure.h
Treasure.o: Treasure.h
ServerThread.o: ServerThread.h
ServerMain.o: ServerThread.h
ClientThread.o: ClientThread.h
ClientMain.o: ClientThread.h

.PHONY: clean mrproper

clean:
	rm -rf *.o

mrproper: clean
	rm -rf $(EXEC)
