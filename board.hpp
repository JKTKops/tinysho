#ifndef BOARD_H
#define BOARD_H

#include "piece.hpp"
#include <cstdint>
#include <iostream>
#include <vector>

/*
Definitions of the board and supporting types, as well as the
board object on which moves will be played out.
*/

namespace Board {
  extern Piece::piece square[25];
  extern bool to_move; // false=SENTE, true=GOTE

  /// Colors in general are just one bit. Piece::SENTE and Piece::GOTE are bitfields,
  /// not usually what we want. So we make Board::SENTE and Board::GOTE too.
  typedef bool color;
  constexpr color SENTE = false;
  constexpr color GOTE = true;
  color piece_color(Piece::piece pt);

  /// **sorted** square indices for the pieces of each color.
  /// must be kept consistent with [square]!
  /// Indexed by color, then vector is of indices.
  extern std::vector<uint8_t> occupancy[2];

  /// player hands: count of pieces of each (unpromoted) type.
  /// For convenience, hand[x][0] is always 0 (corresponds to NO_PIECE).
  /// Indexed by color, then piece type.
  extern uint8_t hand[2][Piece::NB_UNPROMOTED];


  /// We can store moves in relatively few bits but use a
  /// full int anyway because our major space constraints are not
  /// moves. This representation is very similar to stockfish's.
  /// Since we are allowing several bytes anyway, we can put things
  /// up in a struct and byte-align fields for fast extraction.
  struct Move {
    int8_t origin;      // -1 if drop
    int8_t destination; // always in [0,24]
    uint8_t piece_drop; // What piece is being dropped? undef if origin!=-1
    bool promotion;     // moving piece is promoting?

    Move(int8_t orig, int8_t dest, bool promo);
    Move(int8_t dest, Piece::piece_type pt);
  };

  void print_board(std::ostream& os);

  /* Throws illegal_argument if something is wrong.
    The board may still be modified in that case! */
  void importFEN(const std::string& FEN);
  extern std::string startFEN;
}

#endif /* BOARD_H */
