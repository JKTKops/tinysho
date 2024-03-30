#include "piece.hpp"
#include "board.hpp"

namespace Piece {

  piece swap_color(piece pt) {
    return pt ^ PIECE_COLOR_MASK;
  }

  piece color_piece(piece_type pt, Board::color color) {
    if (color) {
      return pt | GOTE;
    } else {
      return pt | SENTE;
    }
  }

  std::ostream& operator<<(std::ostream& os, const Printable pt) {
    static char letters[Piece::PIECE_TYPE_MASK + 1] = {
      0, 'p', 's', 'g', 'b', 'r', 'k', 0,
      0, 't', 'n', 0,   'h', 'd'
    };

    int type = pt & Piece::PIECE_TYPE_MASK;
    char letter = letters[type];

    if (letter == 0) {
      os << " ";
      return os;
    }

    if (pt & Piece::SENTE) {
      os << (char)toupper(letter);
    } else if (pt & Piece::GOTE) {
      os << letter;
    } else {
      os << '?';
    }

    return os;
  }
}

