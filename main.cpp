#include <iostream>
#include "board.hpp"
#include "movegen.hpp"
#include "piece.hpp"
#include "perft.hpp"
#include "immintrin.h"

int main() {
  std::vector<Board::Move> moves;
  Board::StateInfo si[4];

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
  //Board::importFEN("k2T1/2GN1/BS3/b2K1/R4 w PRg");

//  Board::Move m(0,6,true);
//  Board::do_move(m, si[0]);
//  m = Board::Move(19, 23, false);
//  Board::do_move(m, si[1]);
  std::cout << Board::exportFEN() << std::endl;
  Board::print_board(std::cout);
  Board::check_consistency();

  Movegen::allow_drop_pawn_checkmate = true;
  Movegen::Perft::perft(3, true);

  /*
  Board::importFEN("k4/5/5/5/4K b PPSSGGBBRR");
  std::cout << Board::exportFEN() << std::endl;
  Board::print_board(std::cout);
  */
}