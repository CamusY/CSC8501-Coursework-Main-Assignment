#include "TFT.h"
namespace ipd {
    std::string TFT::name() const {
        return "TFT";
    }

    Move TFT::nextMove(const MatchState& state, int selfIndex, Random&) {
        if (!state.hasHistory()) {
            return Move::Cooperate;
        }

        return state.lastOpponentMove(selfIndex);
    }

    double TFT::complexity() const {
        return 2.0;
    }
}