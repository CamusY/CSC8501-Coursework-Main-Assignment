#include "CTFT.h"

namespace ipd {
    CTFT::CTFT()
        : m_contrite(false) {
    }

    std::string CTFT::name() const {
        return "CTFT";
    }

    Move CTFT::nextMove(const MatchState& state, int selfIndex, Random&) {
        if (!state.hasHistory()) {
            m_contrite = false;
            return Move::Cooperate;
        }

        const auto last = state.lastRound();
        const Move myLast = selfIndex == 0 ? last.first : last.second;
        const Move opponentLast = selfIndex == 0 ? last.second : last.first;

        if (m_contrite) {
            if (opponentLast == Move::Cooperate) {
                m_contrite = false;
            }
            return Move::Cooperate;
        }

        if (myLast == Move::Defect && opponentLast == Move::Cooperate) {
            m_contrite = true;
            return Move::Cooperate;
        }

        return opponentLast;
    }

    void CTFT::reset() {
        m_contrite = false;
    }

    double CTFT::complexity() const {
        return 3.0;
    }
}