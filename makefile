CXX=g++
CXXFLAGS=-g3 -O0 -Wall -c -std=gnu++11 -pthread
LDFLAGS=-std=gnu++11 -pthread
EXEC=server client
SRC=$(wildcard *.cpp)
OBJ= $(SRC:.cpp=.o)

all: $(EXEC)

server: mainServer.o ServerThread.o ServerThread.h
	$(CXX) -o $@ $^ $(LDFLAGS)

client: mainClient.o ClientThread.o ClientThread.h
	$(CXX) -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	$(CXX) -o $@ $^ $(CXXFLAGS)

.PHONY: clean mrproper

clean:
	rm -rf *.o

mrproper: clean
	rm -rf $(EXEC)
