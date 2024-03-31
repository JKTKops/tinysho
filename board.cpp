#include "board.hpp"
#include <algorithm>
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
    : origin(orig), destination(dest), piece_drop(Piece::NO_PIECE), promotion(promo)
    { }
  Move::Move(square dest, Piece::piece_type pt)
    : origin(-1), destination(dest), piece_drop(pt), promotion(false)
    { }

  std::ostream& operator<<(std::ostream& os, const Move& move) {
    uint8_t origr = move.origin / 5,
            origf = move.origin % 5,
            destr = move.destination / 5,
            destf = move.destination % 5;

    if (move.piece_drop == Piece::NO_PIECE) {
      os << origf+1 << (char)(origr + 'a') << destf+1 << (char)(destr + 'a');
      if (move.promotion) os << '+';
    } else {
      os << (Piece::Printable)(move.piece_drop) << '*'
         << destf+1 << (char)(destr + 'a');
    }
    return os;
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
    std::string boardFEN  = FEN.substr(0, pos);
    s = s.substr(pos + 1);
    pos = s.find(" ");
    std::string playerFEN = FEN.substr(0, pos);
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

    if (playerFEN.length() != 1 &&
        playerFEN[0] != 'b' &&
        playerFEN[0] != 'w')
    {
      std::cout << playerFEN;
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