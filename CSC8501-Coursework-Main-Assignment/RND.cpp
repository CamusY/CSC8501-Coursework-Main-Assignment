#include "RND.h"

namespace ipd {
    std::string RND::name() const {
        return "RND";
    }

    Move RND::nextMove(const MatchState&, int, Random& rng) {
        return rng.nextBool() ? Move::Cooperate : Move::Defect;
    }

    double RND::complexity() const {
        return 1.5;
    }
}