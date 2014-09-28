CXX=g++
DEBUG=-g3
CXXFLAGS+=${DEBUG} -O3 -Wall -Wextra -c -std=gnu++11 -pthread
LDFLAGS+=-std=gnu++11 -pthread -lncurses
EXEC=maze
SRC=$(wildcard *.cpp)
OBJ= $(SRC:.cpp=.o)
INC=$(wildcard *.h)

all: $(EXEC)

maze: $(OBJ)
	$(CXX) -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	$(CXX) -o $@ $< $(CXXFLAGS)

# Headers dependencies
main.o: ServerThread.h ClientThread.h
GameState.o: GameState.h Cell.h Player.h Treasure.h
Player.o: GameState.h Cell.h Player.h Treasure.h
Cell.o: Cell.h
ServerThread.o: ServerThread.h config.h
ClientThread.o: ClientThread.h config.h
PlayerManager.o: PlayerManager.h
ClientViewNcurses.o: ClientViewNcurses.h ClientView.h ClientThread.h

.PHONY: clean mrproper

clean:
	rm -rf *.o

mrproper: clean
	rm -rf $(EXEC)
