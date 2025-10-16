#include "Move.h"
#include <ostream>
namespace ipd {
    std::string toString(Move move) {
        switch (move) {
        case Move::Cooperate:
            return "C";
        case Move::Defect:
            return "D";
        default:
            return "?";
        }
    }

    Move flip(Move move) {
        return move == Move::Cooperate ? Move::Defect : Move::Cooperate;
    }

    std::ostream& operator<<(std::ostream& os, Move move) {
        return os << toString(move);
	}
}