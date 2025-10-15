#include "GRIM.h"
namespace ipd {
    GRIM::GRIM()
        : m_triggered(false) {
    }

    std::string GRIM::name() const {
        return "GRIM";
    }

    Move GRIM::nextMove(const MatchState& state, int selfIndex, Random&) {
        if (m_triggered) {
            return Move::Defect;
        }
        if (state.hasHistory() && state.lastOpponentMove(selfIndex) == Move::Defect) {
            m_triggered = true;
            return Move::Defect;
        }
        return Move::Cooperate;
    }

    void GRIM::reset() {
        m_triggered = false;
    }

    double GRIM::complexity() const {
        return 2.0;
    }
}