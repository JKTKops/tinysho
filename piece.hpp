#ifndef PIECE_H
#define PIECE_H

#include <iostream>

/*
Minishogi pieces.

(c) Max Kopinsky & Hamza Javed 2024
*/

namespace Piece {
  typedef unsigned piece; /* char? */
  typedef piece piece_type; /* no SENTE | GOTE bits */
  
  constexpr piece NO_PIECE = 0;
  constexpr piece PAWN     = 1;
  constexpr piece SILVER   = 2;
  constexpr piece GOLD     = 3;
  constexpr piece BISHOP   = 4;
  constexpr piece ROOK     = 5;
  constexpr piece KING     = 6;
  constexpr piece NB_UNPROMOTED = KING;

  constexpr piece PROMOTED = 8;
  constexpr piece TOKIN    = PROMOTED | PAWN;
  constexpr piece P_SILVER = PROMOTED | SILVER; // letter is 'n' for narigin
  constexpr piece HORSE    = PROMOTED | BISHOP;
  constexpr piece DRAGON   = PROMOTED | ROOK;

  constexpr piece SENTE = 16;
  constexpr piece GOTE  = 32;

  constexpr int PIECE_TYPE_MASK = 0x0F;
  constexpr int PIECE_COLOR_MASK = SENTE | GOTE;

  piece swap_color(piece pt);
  piece color_piece(piece_type pt, bool color); // color is a Board::color

  enum Printable : piece; // simple overloadable type

  std::ostream& operator<<(std::ostream& os, const Printable pt);

}

#endif /* PIECE_H */