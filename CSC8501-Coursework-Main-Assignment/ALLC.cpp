#include "ALLC.h"
namespace ipd {
    std::string ALLC::name() const {
        return "ALLC";
    }

    Move ALLC::nextMove(const MatchState&, int, Random&) {
        return Move::Cooperate;
    }

    int ALLC::complexity() const {
        return 1;
    }
}
