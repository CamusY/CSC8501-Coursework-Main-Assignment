#include "PAVLOV.h"
namespace ipd {
    std::string PAVLOV::name() const {
        return "PAVLOV";
    }

    Move PAVLOV::nextMove(const MatchState& state, int selfIndex, Random&) {
		(void)selfIndex; // Unused parameter
        if (!state.hasHistory()) {
            m_lastMove = Move::Cooperate;
            return m_lastMove;
        }

        const auto last = state.lastRound();
        const bool bothSame = last.first == last.second;
        if (!bothSame) {
            m_lastMove = (m_lastMove == Move::Cooperate) ? Move::Defect : Move::Cooperate;
        }
        return m_lastMove;
    }

    void PAVLOV::reset() {
        m_lastMove = Move::Cooperate;
    }

    int PAVLOV::complexity() const {
        return 2;
    }
}