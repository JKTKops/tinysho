#include <iostream>
#include "board.hpp"
#include "piece.hpp"

int main() {
  Board::importFEN(Board::startFEN);
  Board::print_board(std::cout);

  Board::importFEN("k2TS/2G2/BS3/b2K1/R4 b PRg");
  Board::print_board(std::cout);

  Board::importFEN("k4/5/5/5/4K b PPSSGGBBRR");
  Board::print_board(std::cout);
}