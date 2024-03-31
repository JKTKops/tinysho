#include <iostream>
#include "board.hpp"
#include "piece.hpp"
#include "movegen.hpp"

int main() {
  std::vector<Board::Move> moves;

  /*
  Board::importFEN(Board::startFEN);
  std::cout << Board::exportFEN() << std::endl;
  Board::print_board(std::cout);

  for (auto& move : moves) {
    std::cout << move << std::endl;
  }
  moves.clear();
  */

  Board::importFEN("k2TS/2G2/BS3/b2K1/R4 b PRg");
  std::cout << Board::exportFEN() << std::endl;
  Board::print_board(std::cout);
  moves = Movegen::pseudolegal();
  for (auto& move : moves) {
    std::cout << move << std::endl;
  }

  /*
  Board::importFEN("k4/5/5/5/4K b PPSSGGBBRR");
  std::cout << Board::exportFEN() << std::endl;
  Board::print_board(std::cout);
  */
}