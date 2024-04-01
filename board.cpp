#include "board.hpp"
#include "movegen.hpp" // for slow checkmate detection, remove later!
#include <algorithm>
#include <cassert>
#include <cstring>
#include <sstream>
#include <iterator>

namespace Board {
  Piece::piece Square[25] = { };
  bool to_move = false;

  std::set<uint8_t> occupancy[2];
  uint8_t hand[2][Piece::NB_UNPROMOTED]{};

  std::vector<color> colors = {SENTE, GOTE};

  Move::Move(square orig, square dest, bool promo)
    : origin(orig), destination(dest), pieceDrop(Piece::NO_PIECE), promotion(promo)
    { }
  Move::Move(square dest, Piece::piece_type pt)
    : origin(-1), destination(dest), pieceDrop(pt), promotion(false)
    { }

  std::ostream& operator<<(std::ostream& os, const Move& move) {
    uint8_t origr = move.origin / 5,
            origf = move.origin % 5,
            destr = move.destination / 5,
            destf = move.destination % 5;

    if (move.pieceDrop == Piece::NO_PIECE) {
      os << origf+1 << (char)(origr + 'a') << destf+1 << (char)(destr + 'a');
      if (move.promotion) os << '+';
    } else {
      os << (Piece::Printable)(move.pieceDrop) << '*'
         << destf+1 << (char)(destr + 'a');
    }
    return os;
  }

  StateInfo *st = NULL;
  StateInfo::StateInfo() : prev(NULL) { }

  /// @brief Evacuate a square, returning the piece that was there.
  Piece::piece evacuate(square sq, color c) {
    Piece::piece p = Board::Square[sq];
    if (p == Piece::NO_PIECE) return p;

    Board::Square[sq] = Piece::NO_PIECE;
    Board::occupancy[c].erase(sq);
    return p;
  }
  Piece::piece evacuate(square sq) {
    Piece::piece p = Board::Square[sq];
    return evacuate(sq, Piece::color(p));
  }

  /// @brief Occupy a square with the given piece.
  void occupy(square sq, Piece::piece p, color c) {
    Board::Square[sq] = p;
    Board::occupancy[c].insert(sq);
  }
  void occupy(square sq, Piece::piece p) {
    occupy(sq, p, Piece::color(p));
  }

  void do_move(Move m, StateInfo& new_st) {
    // We must treat new_st as being completely invalid and initialize anything
    // that we care about.
    new_st.prev = st;
    new_st.capturedPiece = Piece::NO_PIECE;
    st = &new_st;

    color us = Board::to_move;
    color them = !us;

    Board::to_move = them; // swap player to move

    if (m.pieceDrop != Piece::NO_PIECE) {
      // this move is a drop.
      
      // Ensure target square is empty.
      assert(Board::Square[m.destination] == Piece::NO_PIECE);
      // ensure the piece being dropped is our color.
      assert(Piece::color(m.pieceDrop) == us);
      // ensure we have one of these to drop.
      assert(Board::hand[us][Piece::type(m.pieceDrop)] > 0);

      // Remove one of these from our hand.
      Board::hand[us][Piece::type(m.pieceDrop)]--;
      occupy(m.destination, m.pieceDrop, us);
    }
    else {
      // this move is a proper move.

      // ensure there is a moving piece and that it is our color
      assert(Board::Square[m.origin] != Piece::NO_PIECE);
      assert(Piece::color(Board::Square[m.origin]) == us);
      // ensure that if there is a captured piece, it is not our color
      assert(Board::Square[m.destination] == Piece::NO_PIECE
             || Piece::color(Board::Square[m.destination]) == them);

      // Evacuate the origin square
      Piece::piece moving_piece = evacuate(m.origin, us);
      // Evacuate the destination
      Piece::piece captured_piece = evacuate(m.destination, them);

      // If a piece is being captured...
      if (captured_piece != Piece::NO_PIECE) {
        // add one of its piece type to our hand
        Board::hand[us][Piece::type(captured_piece)]++;
        st->capturedPiece = captured_piece;
      }

      // If the moving piece is being promoted...
      if (m.promotion) {
        assert(Piece::can_promote(moving_piece));
        moving_piece = Piece::promote(moving_piece);
      }

      // Occupy the target square.
      occupy(m.destination, moving_piece, us);
    }
  }

  void undo_move(Move m) {
    color them = Board::to_move;
    color us = !them;
    Board::to_move = us;

    if (m.pieceDrop != Piece::NO_PIECE) {
      // undoing a drop.
      // Evacuate the destination square.
      Piece::piece p = evacuate(m.destination, us);
      // Ensure the dropped piece was on the destination square.
      assert(p == m.pieceDrop);
      // Pick up the piece.
      // (using m.pieceDrop instead of p allows instructions to overlap)
      Board::hand[us][Piece::type(m.pieceDrop)]++;
    }
    else {
      // undoing a proper move.
      // Ensure the origin square is empty.
      assert(Board::Square[m.origin] == Piece::NO_PIECE);
      // Evacuate the destination and occupy the origin, possibly demoting.
      Piece::piece p = evacuate(m.destination, us);
      if (m.promotion) p = Piece::demote(p);
      occupy(m.origin, p, us);

      // If this move was a capture, remove the captured piece from our hand
      // and put it back on the destination square with the opponent's color.
      // Take care - the captured piece may have been promoted!
      Piece::piece captured = st->capturedPiece;
      if (captured != Piece::NO_PIECE) {
        Board::hand[us][Piece::type(captured)]--;
        occupy(m.destination, captured, them);
      }
    }

    // unwind stateinfo
    st = st->prev;
  }

  //bool is_checkmate() {
  //}

  void check_consistency() {
    unsigned counts[Piece::KING+1]{};

    for (Board::square sq = 0; sq < 25; ++sq) {
      Piece::piece p = Board::Square[sq];
      if (p != Piece::NO_PIECE) {
        counts[Piece::upt(p)]++;
        Board::color c = Piece::color(p);
        assert(Board::occupancy[c].count(sq));
      }
    }
    for (Board::color c : Board::colors) {
      for (Board::square sq : Board::occupancy[c]) {
        Piece::piece p = Board::Square[sq];
        assert(p != Piece::NO_PIECE);
        assert(Piece::color(p) == c);
      }

      for (int i = Piece::PAWN; i < Piece::NB_UNPROMOTED; ++i) {
        counts[i] += Board::hand[c][i];
      }
    }

    assert(counts[Piece::PAWN] == 2);
    assert(counts[Piece::SILVER] == 2);
    assert(counts[Piece::GOLD] == 2);
    assert(counts[Piece::BISHOP] == 2);
    assert(counts[Piece::ROOK] == 2);
    assert(counts[Piece::KING] == 2);
  }

  bool in_promo_zone(square sq, color c) {
    if (c == SENTE) {
      return sq >= 0 && sq <= 4;
    } else {
      return sq >= 20 && sq <= 24;
    }
  }

  static constexpr int DISPLAY_DUPLICATE = 0x80;

  static void populate_display_hand(
    int display_hand[5], uint8_t hand[Piece::NB_UNPROMOTED]
  ) {
    // display_hand will be populated as RBGSP, which is reverse order for sente.
    unsigned ix = 0;
    for (Piece::piece pt = Piece::ROOK; pt >= Piece::PAWN; --pt) {
      if (hand[pt]) {
        display_hand[ix] = pt;
        if (hand[pt] > 1) {
          display_hand[ix] |= DISPLAY_DUPLICATE;
        }
        ++ix;
      }
    }
  }

  void print_board(std::ostream& os) {
    // figure out how to print hands. 128s bits are used for duplicates.
    int gote_display_hand[5]{};
    int sente_display_hand[5]{};

    populate_display_hand(sente_display_hand, hand[0]);
    populate_display_hand(gote_display_hand, hand[1]);
    std::reverse(std::begin(sente_display_hand), std::end(sente_display_hand));

    os << "   +---+---+---+---+---+   " << std::endl;
    for (unsigned row = 0; row < 5; ++row) {
      Piece::piece in_hand = gote_display_hand[row] & ~DISPLAY_DUPLICATE;
      os << (Piece::Printable)Piece::color_piece(in_hand, Board::GOTE);
      os << (gote_display_hand[row] & DISPLAY_DUPLICATE ? '2' : ' ') << " | ";
      for (int col = 4; col >= 0; --col) {
        Piece::piece pt = Board::Square[row*5 + col];
        os << (Piece::Printable)pt << " | ";
      }

      in_hand = sente_display_hand[row] & ~DISPLAY_DUPLICATE;
      os << (Piece::Printable)Piece::color_piece(in_hand, Board::SENTE);
      if (sente_display_hand[row] & DISPLAY_DUPLICATE) os << '2';
      os << std::endl;
    }
    os << "   +---+---+---+---+---+" << std::endl;
  }

  /// Print the current position to a std::string in FEN notation.
  std::string exportFEN() {
    std::ostringstream ss;

    // board part
    for (unsigned row = 0; row < 5; ++row) {
      if (row != 0) ss << "/";
      unsigned blanks = 0;
      for (int col = 4; col >= 0; --col) {
        Piece::piece pt = Board::Square[row*5 + col];

        if (pt == Piece::NO_PIECE) {
          ++blanks;
        } else {
          if (blanks != 0) {
            ss << blanks;
            blanks = 0;
          }
          ss << (Piece::Printable)pt;
        }
      }
      if (blanks != 0) ss << blanks;
    }

    // player part
    if (Board::to_move == Board::SENTE) {
      ss << " b ";
    } else {
      ss << " w ";
    }

    // hand part

    bool empty_hand = true;
    for (Board::color color : colors) {
      for (Piece::piece_type pt = Piece::PAWN; pt < Piece::NB_UNPROMOTED; ++pt) {
        for (unsigned count = 0; count < hand[color][pt]; ++count) {
          ss << (Piece::Printable)Piece::color_piece(pt, color);
          empty_hand = false;
        }
      }
    }
    if (empty_hand) ss << "-";

    return ss.str();
  }

  // the "standard" notation is to use +P for T etc, but that doesn't look good
  // in ascii board outputs and I'd rather make the input and output match. 
  void importFEN(const std::string& FEN) {
    static Piece::piece piece_table[256] = {
      ['K'] = Piece::SENTE | Piece::KING,   ['k'] = Piece::GOTE | Piece::KING,
      ['P'] = Piece::SENTE | Piece::PAWN,   ['p'] = Piece::GOTE | Piece::PAWN,
      ['T'] = Piece::SENTE | Piece::TOKIN,  ['t'] = Piece::GOTE | Piece::TOKIN,
      ['S'] = Piece::SENTE | Piece::SILVER, ['s'] = Piece::GOTE | Piece::SILVER,
      ['N'] = Piece::SENTE | Piece::P_SILVER, ['n'] = Piece::GOTE | Piece::P_SILVER,
      ['G'] = Piece::SENTE | Piece::GOLD,   ['g'] = Piece::GOTE | Piece::GOLD,
      ['B'] = Piece::SENTE | Piece::BISHOP, ['b'] = Piece::GOTE | Piece::BISHOP,
      ['R'] = Piece::SENTE | Piece::ROOK,   ['r'] = Piece::GOTE | Piece::ROOK,
      ['H'] = Piece::SENTE | Piece::HORSE,  ['h'] = Piece::GOTE | Piece::HORSE,
      ['D'] = Piece::SENTE | Piece::DRAGON, ['d'] = Piece::GOTE | Piece::DRAGON,
    };

    std::string s = FEN;
    auto pos = s.find(" ");
    std::string boardFEN  = s.substr(0, pos);
    s = s.substr(pos + 1);
    pos = s.find(" ");
    std::string playerFEN = s.substr(0, pos);
    s = s.substr(pos + 1);
    std::string handFEN = s;

    int file = 4;
    int rank = 0;

    std::memset(Board::Square, 0, sizeof(Board::Square));
    for (char c : boardFEN) {
      if (c == '/') {
        if (file != -1) throw std::invalid_argument("not 5 items in rank");
        file = 4;
        ++rank;
        continue;
      }

      if (c >= '1' && c <= '5') {
        file -= (c - '0');
      } else {
        Piece::piece pt = piece_table[c];
        if (pt == Piece::NO_PIECE || file < 0 || rank > 4) {
          throw std::invalid_argument("invalid FEN item (board)");
        }
        Board::square sq = rank * 5 + file;
        Board::Square[sq] = pt;
        Board::occupancy[Piece::color(pt)].insert(sq);
        --file;
      }
    }

    if (playerFEN.length() != 1 ||
        (playerFEN[0] != 'b' &&
         playerFEN[0] != 'w'))
    {
      std::cerr << playerFEN;
      throw std::invalid_argument("malformed player-to-move");
    } else if (playerFEN[0] == 'b') {
      Board::to_move = false;
    } else if (playerFEN[0] == 'w') {
      Board::to_move = true;
    }

    std::memset(Board::hand, 0, sizeof(Board::hand));
    if (handFEN.length() != 1 || handFEN[0] != '-') {
      for (char c : handFEN) {
        Piece::piece pt = piece_table[c];
        if (pt == Piece::NO_PIECE || pt & Piece::PROMOTED) {
          throw std::invalid_argument("invalid FEN item (hand)");
        }

        Board::hand[Piece::color(pt)][Piece::type(pt)]++;
      }
    }
  }

  std::string startFEN = "rbsgk/4p/5/P4/KGSBR b -";
  
}