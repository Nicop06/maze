CXX=g++
CXXFLAGS=-g3 -O0 -Wall -c -std=c++1y -pthread
LDFLAGS=-pthread
EXEC=test
SRC=$(wildcard *.cpp)
OBJ= $(SRC:.cpp=.o)

all: $(EXEC)

test: $(OBJ)
	$(CXX) -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	$(CXX) -o $@ $^ $(CXXFLAGS)

.PHONY: clean mrproper

clean:
	rm -rf *.o

mrproper: clean
	rm -rf $(EXEC)
