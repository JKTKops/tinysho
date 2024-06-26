CPP = clang++
# add -DNDEBUG to OPT_ARGS to disable assertions
OPT_ARGS = -O2 -march=native
OBJECT_ARGS = -c $(OPT_ARGS)
MAIN_ARGS = $(OPT_ARGS)

main: board.o piece.o movegen.o perft.o main.cpp
	$(CPP) $(MAIN_ARGS) main.cpp board.o piece.o movegen.o -o main

board.o: piece.hpp board.hpp board.cpp
	$(CPP) $(OBJECT_ARGS) board.cpp -o board.o

piece.o: piece.hpp piece.cpp
	$(CPP) $(OBJECT_ARGS) piece.cpp -o piece.o

movegen.o: piece.hpp board.hpp movegen.hpp movegen.cpp
	$(CPP) $(OBJECT_ARGS) movegen.cpp -o movegen.o

perft.o: board.hpp movegen.hpp perft.hpp
	$(CPP) $(OBJECT_ARGS) perft.hpp -o perft.o
