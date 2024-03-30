CPP = clang++
OPT_ARGS = -O2
OBJECT_ARGS = -c $(OPT_ARGS)
MAIN_ARGS = $(OPT_ARGS)

main: board.o piece.o main.cpp
	$(CPP) $(MAIN_ARGS) main.cpp board.o piece.o -o main

board.o: board.hpp board.cpp
	$(CPP) $(OBJECT_ARGS) board.cpp -o board.o

piece.o: piece.hpp piece.cpp
	$(CPP) $(OBJECT_ARGS) piece.cpp -o piece.o
