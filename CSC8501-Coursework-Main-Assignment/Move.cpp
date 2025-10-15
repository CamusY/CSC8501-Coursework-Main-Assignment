#include "Move.h"
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
}