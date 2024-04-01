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

  // this one has a potential drop pawn checkmate
  //Board::importFEN("k2TS/2G2/BS3/b2K1/R4 b PRg");

  // this one has perft(2) < perft(1)
  //Board::importFEN("k3S/B1GP1/5/GS1K1/R1B2 b RP");

  // sente starts in check here
  Board::importFEN("2k1S/B1rP1/2KG1/GS1p1/R1B2 b -");

  //Board::Move m(8, Piece::color_piece(Piece::PAWN, Board::SENTE));
  //Board::do_move(m, si[0]);
  //m = Board::Move(19, 23, false);
  //Board::do_move(m, si[1]);
  std::cout << Board::exportFEN() << std::endl;
  Board::print_board(std::cout);
  Board::check_consistency();

  Movegen::allow_drop_pawn_checkmate = true;
  Movegen::Perft::perft(4, true);

  /*
  Board::importFEN("k4/5/5/5/4K b PPSSGGBBRR");
  std::cout << Board::exportFEN() << std::endl;
  Board::print_board(std::cout);
  */
}