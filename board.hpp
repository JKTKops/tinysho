#pragma once

#include "piece.hpp"
#include <cstdint>
#include <iostream>
#include <vector>
#include <set>

/*
Definitions of the board and supporting types, as well as the
board object on which moves will be played out.
*/

namespace Board {
  extern Piece::piece Square[25];
  typedef uint8_t square;

  /// Colors in general are just one bit. Piece::SENTE and Piece::GOTE are bitfields,
  /// not usually what we want. So we make Board::SENTE and Board::GOTE too.
  typedef bool color;
  extern color to_move; // false=SENTE, true=GOTE
  constexpr color SENTE = false;
  constexpr color GOTE = true;
  extern std::vector<color> colors;

  /// **sorted** square indices for the pieces of each color.
  /// must be kept consistent with [square]!
  /// Indexed by color, then vector is of indices.
  extern std::set<square> occupancy[2];

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
    square origin;      // undefined if drop is not NO_PIECE
    square destination; // always in [0,24]
    uint8_t pieceDrop;  // What piece is being dropped?
    bool promotion;     // moving piece is promoting?

    //Move() = default;
    Move(square orig, square dest, bool promo = false);
    Move(square dest, Piece::piece_type pt);

    friend std::ostream& operator<<(std::ostream& os, const Move& move);
  };

  /// There is some extra state associated with a board, in particular with the
  /// last move. More can easily be added here in the future. For now, we track
  /// what piece the previous move captured (perhaps none!) so that we can undo
  /// moves later.
  /// StateInfo objects form a linked list which will generally consist entirely
  /// of stack objects.
  class StateInfo {
  public:
    StateInfo *prev;
    Piece::piece capturedPiece = Piece::NO_PIECE;

    StateInfo();
    StateInfo(StateInfo& si) = default;
    StateInfo(StateInfo&& si) = default;
  };
  extern StateInfo *st;

  void do_move(Move m, StateInfo& new_st);
  void undo_move(Move m);

  bool is_checkmate();

  void check_consistency();

  bool in_promo_zone(square sq, color c);

  void print_board(std::ostream& os);

  std::string exportFEN();

  /* Throws illegal_argument if something is wrong.
    The board may still be modified in that case! */
  void importFEN(const std::string& FEN);
  extern std::string startFEN;
}
