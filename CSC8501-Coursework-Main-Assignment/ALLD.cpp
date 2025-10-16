#include "ALLD.h"
namespace ipd {
    std::string ALLD::name() const {
        return "ALLD";
    }

    Move ALLD::nextMove(const MatchState&, int, Random&) {
        return Move::Defect;
    }

    int ALLD::complexity() const {
        return 1;
    }
}