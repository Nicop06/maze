CXX=g++
DEBUG=-g3
CXXFLAGS+=${DEBUG} -O3 -Wall -Wextra -c -std=gnu++11 -pthread
LDFLAGS+=-std=gnu++11 -pthread -lncurses
EXEC=maze
SRC=$(wildcard *.cpp)
OBJ= $(SRC:.cpp=.o)
INC=$(wildcard *.h)
ARC=A0119965_A0119984.zip

all: $(EXEC)

maze: $(OBJ)
	$(CXX) -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	$(CXX) -MMD -o $@ $< $(CXXFLAGS)

-include *.d

.PHONY: clean mrproper

dist:
	zip $(ARC) $(SRC) $(INC) Makefile AssignmentOneSubmissionSummary.txt

clean:
	rm -rf *.o *.d

mrproper: clean
	rm -rf $(EXEC) $(ARC)
