#include "movegen.hpp"
#include <algorithm>
#include <bit>

#include <sstream>

namespace Movegen {

  /* Movement Generation slow version 1
  The main idea is to start with pseudo-legal moves, then use slow
  legality testing. Lucky us, we don't have to worry about tricky
  garbage like castling rights or EP.

  Generators for subsets of legal moves just generate the pseudo-legal
  moves, filter out the ones meeting the appropriate conditions, then
  filter out the legal ones.
  */

  // main idea of sliding move generation from Sebastian Lague
  // "Coding Adventure: Chess" on youtube
  enum direction {
    NORTH,
    SOUTH,
    EAST,
    WEST,
    NORTH_EAST,
    NORTH_WEST,
    SOUTH_EAST,
    SOUTH_WEST,
    NB_DIRECTIONS,
  };
  static const int direction_offsets[8] = {
    -5, 5, -1, 1, -6, -4, 4, 6
  };
  // precomputed distance of each square to the nearest edge in each direction.
  static int num_squares_to_edge[25][8] {};
  // we want to populate num_squares_to_edge statically so we use this
  // ugly trick to call a function at startup.
  int populate_num_sq2e() {
    for (int file = 0; file < 5; ++file) {
      for (int rank = 0; rank < 5; ++rank) {
        int num_north = rank;
        int num_south = 4-rank;
        int num_east = file;
        int num_west = 4-file;

        Board::square sq = rank * 5 + file;
        num_squares_to_edge[sq][NORTH] = num_north;
        num_squares_to_edge[sq][SOUTH] = num_south;
        num_squares_to_edge[sq][EAST]  = num_east;
        num_squares_to_edge[sq][WEST]  = num_west;
        num_squares_to_edge[sq][NORTH_EAST] = std::min(num_north, num_east);
        num_squares_to_edge[sq][NORTH_WEST] = std::min(num_north, num_west);
        num_squares_to_edge[sq][SOUTH_EAST] = std::min(num_south, num_east);
        num_squares_to_edge[sq][SOUTH_WEST] = std::min(num_south, num_west);
      }
    }
    return 0;
  }
  int _unused = populate_num_sq2e();

#define CLSB(x)    ((x) & (x-1))

  // directions that each piece is capable of stepping
  // represented as 8 bits, one per direction, LSB is North,
  // MSB is southwest. We can iterate directions effeciently
  // with std::countr_zero and CLSB.
  uint8_t steps_of[2][Piece::NB_PIECE_TYPES] = {
    { /* ******* SENTE ******* */
      /* NO_PIECE */ 0,
      /* PAWN */     0b00000001,
      /* SILVER */   0b11110001,
      /* GOLD */     0b00111111,
      /* BISHOP */   0,
      /* ROOK */     0,
      /* KING */     0b11111111,
      /* UNUSEDx2 */ 0, 0,
      /* TOKIN */    0b00111111,
      /* P_SILVER */ 0b00111111,
      /* UNUSED */   0,
      /* HORSE */    0b00001111,
      /* DRAGON */   0b11110000,
    },
    { /* ******* GOTE ******* */
      /* NO_PIECE */ 0,
      /* PAWN */     0b00000010,
      /* SILVER */   0b11110010,
      /* GOLD */     0b11001111,
      /* BISHOP */   0,
      /* ROOK */     0,
      /* KING */     0b11111111,
      /* UNUSEDx2 */ 0, 0,
      /* TOKIN */    0b11001111,
      /* P_SILVER */ 0b11001111,
      /* UNUSED */   0,
      /* HORSE */    0b00001111,
      /* DRAGON */   0b11110000,
    }
  };

  direction& operator++(direction& d) {
    d = (direction)(d + 1);
    return d;
  }

  /* Alright, now slow movegen logic. */

  Board::color us;
  Board::color them;

  void sync_colors() {
    us = Board::to_move;
    them = !us;
  }

  void generate_sliding_moves(
    Board::square orig, Piece::piece p,
    std::vector<Board::Move>& moves
  ) {
    direction start_dir = Piece::upt(p) == Piece::BISHOP ? NORTH_EAST : NORTH;
    direction end_dir   = Piece::upt(p) == Piece::ROOK   ? NORTH_EAST : NB_DIRECTIONS;
    bool promotable = Piece::can_promote(p);

    for (direction dir = start_dir; dir < end_dir; ++dir) {
      for (int n = 0; n < num_squares_to_edge[orig][dir]; ++n) {
        Board::square dest = orig + direction_offsets[dir] * (n+1);
        Piece::piece dst_pc = Board::Square[dest];

        // blocked by friendly piece?
        if (Piece::is_color(dst_pc, us)) break;

        moves.push_back(Board::Move(orig, dest, false));
        if (promotable && 
            (Board::in_promo_zone(orig, us) ||
             Board::in_promo_zone(dest, us))
        ) {
          moves.push_back(Board::Move(orig, dest, true));
        }

        // that move would be a capture, can't go further
        if (Piece::is_color(dst_pc, them)) break;
      }
    }
  }

  void generate_step_moves(
    Board::square orig, Piece::piece p,
    std::vector<Board::Move>& moves
  ) {
    bool promotable = Piece::can_promote(p);
    uint8_t step_mask = steps_of[us][Piece::type(p)];
    for ( ; step_mask; step_mask = CLSB(step_mask)) {
      direction d = (direction)std::__countr_zero(step_mask);
      // can step in that direction?
      if (num_squares_to_edge[orig][d] == 0) continue;

      Board::square dest = orig + direction_offsets[d];
      // if there's a friendly piece there, can't do it
      if (Piece::is_color(Board::Square[dest], us)) continue;

      // otherwise, we can do it. And might also be able to promo.
      // If this is a pawn, might **have** to promo.
      if (Piece::type(p) == Piece::PAWN) {
        if (Board::in_promo_zone(dest, us)) {
          moves.push_back(Board::Move(orig, dest, true));
        } else {
          moves.push_back(Board::Move(orig, dest, false));
        }
      }
      else { /* Not a pawn. */
        moves.push_back(Board::Move(orig, dest, false));
        if (promotable && 
            (Board::in_promo_zone(orig, us) ||
             Board::in_promo_zone(dest, us))
        ) {
          moves.push_back(Board::Move(orig, dest, true));
        }
      }
    }
  }

  bool allow_drop_pawn_checkmate = false;
  Board::square king_sq(Board::color c);
  bool is_check(Board::square ksq);
  bool is_not_legal(Board::Move m, Board::square ksq);
  bool pawn_drop_is_checkmate(Board::Move m, Board::square their_king);
  /// helper for generate_drops which generates pawn drops. This requires
  /// checking that we do not nifu, that we would not drop pawns on the
  /// back rank, and that pawn drops which attack the king are not checkmate.
  void generate_pawn_drops(uint8_t* hand, std::vector<Board::Move>& moves) {
    // which files do we already have pawns on?
    bool file_half_closed[5]{};
    for (Board::square sq : Board::occupancy[us]) {
      if (Board::Square[sq] == Piece::color_piece(Piece::PAWN, us)) {
        file_half_closed[sq % 5] = true;
      }
    }

    // find our opponent's king, which we will need for avoiding drop-pawn mates
    Board::square their_king = king_sq(them);
    // Which way do our pawns step?
    direction pawn_atk_dir = (us == Board::SENTE) ? NORTH : SOUTH;
    // Which rank should we start dropping on?
    // Add 5 to start square for search if we are sente. We will search 4 ranks,
    // either 0-3 or 1-4.
    Board::square sq = (us == Board::SENTE) ? 5 : 0;

    for (unsigned rank = 0; rank < 4; ++rank) { // 4, not 5!
      for (unsigned file = 0; file < 5; ++file, ++sq) {
        // don't generate drops on half-closed files (nifu rule)
        if (file_half_closed[file] || Board::Square[sq] != Piece::NO_PIECE) {
          continue;
        }
        // construct the pawn drop
        Board::Move m(sq, Piece::color_piece(Piece::PAWN, us));
        // which square does the pawn attack?
        Board::square atk_sq = sq + direction_offsets[pawn_atk_dir];
        // if that's the opponent's king, we have to see if it is checkmate.
        if (!allow_drop_pawn_checkmate &&
            atk_sq == their_king &&
            pawn_drop_is_checkmate(m, their_king)) {
          continue;
        }
        // Otherwise we can drop this pawn.
        moves.push_back(m);
      }
    }

  }

  bool pawn_drop_is_checkmate(Board::Move m, Board::square their_king) {
    Board::StateInfo si;
    Board::do_move(m, si);
    sync_colors();
    // generate our opponent's check escapes. But don't use 'check_escapes'
    // because the current implementation of it is garbage.
    // Here we just generate all the sliding/step moves, and then we will
    // only check the ones with potential: king moves and captures of the pawn.
    std::vector<Board::Move> opp_moves;
    for (auto sq : Board::occupancy[us]) {
      Piece::piece piece = Board::Square[sq];
      if (Piece::is_sliding_piece(piece)) {
        generate_sliding_moves(sq, piece, opp_moves);
      }
      generate_step_moves(sq, piece, opp_moves);
    }
    bool can_escape = false;
    for (Board::Move opp_move : opp_moves) {
      if (opp_move.origin != their_king && // they move their king
          opp_move.destination != m.destination) { // they capture our pawn
        continue; // if it's neither of those, skip!
      }
      // this is a candidate move for a check escape. If it's legal,
      // so is our pawn drop.
      if (!is_not_legal(opp_move, their_king)) {
        can_escape = true;
        break;
      }
      // is_not_legal being called on our opponent's moves resyncs the colors
      // to us, even after we sync it to the opponent to generate their responses.
      // There's always at least one candidate move: king takes pawn.
    }
    Board::undo_move(m);
    return !can_escape;
  }

  /// This is not the [drops] function in movegen.hpp because
  /// it can generate drops that do not block a check. That is,
  /// it can generate drops that are only pseudolegal.
  void generate_drops(
    std::vector<Board::Move>& moves
  ) {
    uint8_t* hand = Board::hand[us];
    // try dropping every piece in our hand on every available square.
    // catches: cannot drop pawns in promo zone or nifu or checkmate
    if (hand[Piece::PAWN] > 0) generate_pawn_drops(hand, moves);

    for (Piece::piece_type pt = Piece::PAWN+1; pt < Piece::NB_UNPROMOTED; ++pt) {
      // none of this piece in hand
      if (hand[pt] == 0) continue;

      for (Board::square sq = 0; sq < 25; ++sq) {
        if (Board::Square[sq] != Piece::NO_PIECE) continue;
        moves.push_back(Board::Move(sq, Piece::color_piece(pt, us)));
      }
    }
  }

  std::vector<Board::Move> pseudolegal() {
    std::vector<Board::Move> moves;
    sync_colors();

    // iterate over our color's occupancy to find pieces
    for (auto sq : Board::occupancy[us]) {
      Piece::piece piece = Board::Square[sq];
      if (Piece::is_sliding_piece(piece)) {
        generate_sliding_moves(sq, piece, moves);
      }
      generate_step_moves(sq, piece, moves);
    }
    generate_drops(moves);
    return moves;
  }

  Board::square king_sq(Board::color c) {
    for (auto sq : Board::occupancy[c]) {
      Piece::piece p = Board::Square[sq];
      if (p == Piece::color_piece(Piece::KING, c)) {
        return sq;
      }
    }
    throw std::invalid_argument("position has no king?");
  }

  /// VERY SLOW check check. See if we would be in check if we leave the board
  /// in the given state (Board::to_move should be the opponent).
  /// Generate all of our opponent's (pseudolegal) moves, and see if any of them
  /// have our king's square as a destination.
  bool is_check(Board::square king_square) {
    Board::StateInfo si;
    sync_colors();

    std::vector<Board::Move> opp_moves;
    // pseudolegal() but without the drops, which can't take our king
    for (auto sq : Board::occupancy[Board::to_move]) {
      Piece::piece piece = Board::Square[sq];
      if (Piece::is_sliding_piece(piece)) {
        generate_sliding_moves(sq, piece, opp_moves);
      }
      generate_step_moves(sq, piece, opp_moves);
    }

    bool checked = false;
    for (Board::Move opp_move : opp_moves) {
      if (opp_move.destination == king_square) {
        //std::cerr << m << " is check because of " << opp_move << std::endl;
        checked = true;
        break;
      }
    }

    return checked;
  }

  bool is_not_legal(Board::Move m, Board::square king_square) {
    Board::StateInfo si;

    // If the move moved our king, update king_square
    if (m.origin == king_square) {
      king_square = m.destination;
    }

    Board::do_move(m, si);
    bool result = is_check(king_square);
    Board::undo_move(m);

    return result;
  }

  std::vector<Board::Move> legal() {
    std::vector<Board::Move> moves = pseudolegal();
    Board::square our_king = king_sq(us);

    moves.erase(
      std::remove_if(
        moves.begin(), moves.end(),
        [our_king](auto m){return is_not_legal(m, our_king);}
      ),
      moves.end()
    );
    return moves;
  }
}
