#pragma once

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
  // not actually how many pieces there are, since values 7, 8, 11 are unused.
  // The point is that lookup tables with entries for each piece type can use
  // this as their size.
  constexpr int NB_PIECE_TYPES = DRAGON + 1;

  constexpr piece SENTE = 16;
  constexpr piece GOTE  = 32;

  constexpr int PIECE_TYPE_MASK = 0x0F;
  constexpr int PIECE_COLOR_MASK = SENTE | GOTE;

  piece swap_color(piece pt);
  piece color_piece(piece_type pt, bool color); // color is a Board::color
  bool  color(piece pt); // output is a Board::color
  bool  is_color(piece pt, bool color); // color is a Board::color
  piece promote(piece pt);
  piece demote(piece pt);
  piece_type type(piece pt);

  // get the "unpromoted" type of a piece
  static inline piece_type upt(piece pt) {
    return type(demote(pt));
  }

  // accepts a piece or a piece_type
  static inline bool is_sliding_piece(piece pt) {
    piece_type upt = pt & 0x7; // unpromoted piece type
    return upt == BISHOP || upt == ROOK;
  }
  static inline bool is_promoted(piece pt) {
    return pt & PROMOTED;
  }
  static inline bool can_promote(piece pt) {
    pt = type(pt);
    return pt == PAWN || pt == SILVER || pt == BISHOP || pt == ROOK;
  }

  enum Printable : piece; // simple overloadable type

  std::ostream& operator<<(std::ostream& os, const Printable pt);

}
