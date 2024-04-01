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
      moves.push_back(Board::Move(orig, dest, false));
      if (promotable && 
          (Board::in_promo_zone(orig, us) ||
           Board::in_promo_zone(dest, us))
      ) {
        moves.push_back(Board::Move(orig, dest, true));
      }
    }
  }

  // This is not the [drops] function specified in movegen.hpp
  // because [drops] cannot emit a dropped-pawn checkmate.
  // But this function might.
  void generate_drops(
    std::vector<Board::Move>& moves
  ) {
    // try dropping every piece in our hand on every available square.
    // catches: cannot drop pawns in promo zone
    // dropping pawns to give checkmate is considered pseudolegal.
    // is_legal will have to eliminate it later.
    for (Board::square sq = 0; sq < 25; ++sq) {
      if (Board::Square[sq] != Piece::NO_PIECE) continue;

      uint8_t* hand = Board::hand[us];
      for (Piece::piece_type pt = Piece::PAWN; pt < Piece::NB_UNPROMOTED; ++pt) {
        // none of this piece in hand
        if (hand[pt] == 0) continue;
        // would drop a pawn into promotion zone
        if (pt == Piece::PAWN && Board::in_promo_zone(sq, us)) continue;

        moves.push_back(Board::Move(sq, Piece::color_piece(pt, us)));
      }
    }
  }

  std::vector<Board::Move> pseudolegal() {
    us = Board::to_move;
    them = !us;
    std::vector<Board::Move> moves;

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

  Board::square our_king_sq() {
    for (auto sq : Board::occupancy[us]) {
      Piece::piece p = Board::Square[sq];
      if (p == Piece::color_piece(Piece::KING, us)) {
        return sq;
      }
    }
    throw std::invalid_argument("position has no king?");
  }

  bool allow_drop_pawn_checkmate = false;
  /// VERY SLOW legality check. Check if move is legal by making it, generating
  /// all of our opponent's (pseudolegal) moves, and checking if any of them
  /// have our king's square as a destination. Pseudolegal moves capturing
  /// our king are enough to make our move illegal, as we were left in check.
  /// Finally, if our move was a pawn drop that attacked the king, check if
  /// the resulting position is checkmate by searching for pseudolegal moves
  /// that capture the pawn or move the king and recursively testing if they
  /// are legal.
  bool is_legal(Board::Move m, Board::square king_square) {
    Board::StateInfo si;
    Board::do_move(m, si);
    us = Board::to_move;
    them = !us;
    // if they moved their king, update where we think the king is
    if (king_square == m.origin) {
      king_square = m.destination;
    }

    std::vector<Board::Move> opp_moves;
    // pseudolegal() but without the drops, which can't take our king
    for (auto sq : Board::occupancy[Board::to_move]) {
      Piece::piece piece = Board::Square[sq];
      if (Piece::is_sliding_piece(piece)) {
        generate_sliding_moves(sq, piece, opp_moves);
      }
      generate_step_moves(sq, piece, opp_moves);
    }

    bool king_is_dead = false;
    bool pawn_drop_mate = false;
    for (Board::Move opp_move : opp_moves) {
      if (opp_move.destination == king_square) {
        //std::cerr << m << " is illegal because of " << opp_move << std::endl;
        king_is_dead = true;
        goto done;
      }
    }

    if (Piece::type(m.pieceDrop) == Piece::PAWN
        && !allow_drop_pawn_checkmate) {
      // move dropped a pawn and we have to check legality
      int drop_color = Piece::PIECE_COLOR_MASK & m.pieceDrop;
      direction pawn_atk_dir = drop_color == Piece::SENTE
        ? NORTH : SOUTH;
      Board::square pawn_atk = m.destination + direction_offsets[pawn_atk_dir];
      Piece::piece atk_piece = Board::Square[pawn_atk];
      if (Piece::type(atk_piece) != Piece::KING ||
          (atk_piece & Piece::PIECE_COLOR_MASK) == drop_color) {
        goto done;
      }
      // move dropped a pawn and attacked opponent's king.
      // recursively test legality of opponent moves that move their king or
      // capture our pawn. If any are legal, this isn't mate.
      // We didn't generate drops, so we can't possibly inspect an opponent's
      // move that drops a pawn attacking _our_ king, leading to more layers
      // of recursion. Even if we did, that drop from them would not be legal,
      // since their king would still be attacked.
      pawn_drop_mate = true;
      for (Board::Move opp_move : opp_moves) {
        if (opp_move.destination == m.destination || opp_move.origin == pawn_atk) {
          if (is_legal(opp_move, pawn_atk)) {
            pawn_drop_mate = false;
            goto done;
          }
        }
      }
    }

  done:
    Board::undo_move(m);
    return !king_is_dead && !pawn_drop_mate;
  }

  std::vector<Board::Move> legal() {
    std::vector<Board::Move> moves = pseudolegal();
    Board::square our_king = our_king_sq();

    moves.erase(
      std::remove_if(
        moves.begin(), moves.end(),
        [our_king](auto m){return !is_legal(m, our_king);}
      ),
      moves.end()
    );
    return moves;
  }
}
