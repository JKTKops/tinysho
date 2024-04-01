#pragma once

#include "board.hpp"
#include "piece.hpp"
#include <vector>

namespace Movegen {
  /// Generate all pseudo-legal moves.
  std::vector<Board::Move> pseudolegal();
  /// Generate all legal moves.
  std::vector<Board::Move> legal();
  /// Generate all drops.
  std::vector<Board::Move> drops();
  /// Generate all potential checks.
  std::vector<Board::Move> checks();
  /// Generate all potential captures.
  std::vector<Board::Move> captures();
  /// Generate "quiet" moves - not captures or
  /// promotions. Include checks?
  std::vector<Board::Move> quiet();
  /// Generate escapes from checks
  /// (what will this do if position is not check?)
  std::vector<Board::Move> check_escapes();

  extern bool allow_drop_pawn_checkmate;
}
