#pragma once

#include "movegen.hpp"

/* Header-only perft engine for testing movegen. */

namespace Movegen {
namespace Perft {

uint64_t perft(int depth, bool display = false) {
  std::vector<Board::Move> moves;
  uint64_t nodes = 0;

  if (display) {
    std::cout << "perft(" << depth << ") for position " << Board::exportFEN() << std::endl;
    std::cout << "----------------------------------------------------------" << std::endl;
  }

  if (depth == 0) return 1;
  moves = legal();
  if (depth == 1 && !display) {
    return moves.size();
  }

  for (int i = 0; i < moves.size(); ++i) {
    // note that 'legal' has already do_move'd every move in the vector
    // as part of checking if it was legal. So we're repeating work here!
    // This inherent slowdown is easy to fix once we are using a proper
    // legal move generator (i.e. doesn't try to move pinned pieces).
    // Checkmate-pawndrops will probably always be detected by making the
    // move and testing if it is checkmate in whatever way becomes normal -
    // this can happen at most once per node so isn't particularly hot.
    Board::StateInfo si;
    Board::do_move(moves[i], si);
    uint64_t here = perft(depth - 1, false);
    nodes += here;
    Board::undo_move(moves[i]);

    if (display) {
      std::cout << moves[i] << ": " << here << std::endl;
    }
  }

print_results:
  if (display) {
    std::cout << "total: " << nodes << std::endl;
  }

  return nodes;
}

}
}