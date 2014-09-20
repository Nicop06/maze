CXX=g++
DEBUG=-g3
CXXFLAGS+=${DEBUG} -O3 -Wall -Wextra -c -std=gnu++11 -pthread
LDFLAGS+=-std=gnu++11 -pthread -lncurses
EXEC=maze_server maze_client
SRC=$(wildcard *.cpp)
OBJ= $(SRC:.cpp=.o)
INC=$(wildcard *.h)

SERVER_SRC=mainServer.cpp ServerThread.cpp GameState.cpp Player.cpp Cell.cpp PlayerManager.cpp
SERVER_OBJ=$(SERVER_SRC:.cpp=.o)

CLIENT_SRC=mainClient.cpp ClientThread.cpp ClientViewNcurses.cpp
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
Cell.o: Cell.h
ServerThread.o: ServerThread.h config.h
ServerMain.o: ServerThread.h
ClientThread.o: ClientThread.h config.h
ClientMain.o: ClientThread.h ClientView.h
PlayerManager.o: PlayerManager.h
ClientViewNcurses.o: ClientViewNcurses.h ClientView.h ClientThread.h

.PHONY: clean mrproper

clean:
	rm -rf *.o

mrproper: clean
	rm -rf $(EXEC)
