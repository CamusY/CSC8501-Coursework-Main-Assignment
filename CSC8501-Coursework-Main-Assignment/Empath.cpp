#include "Empath.h"

#include <algorithm>

namespace ipd {
    Empath::Empath()
        : m_remorseRemaining(0)
        , m_postRemorseBias(false) {
    }

    void Empath::reset() {
        m_remorseRemaining = 0;
        m_postRemorseBias = false;
    }

    Move Empath::nextMove(const MatchState& state, int selfIndex, Random& rng) {
        if (!state.hasHistory()) {
            reset();
            return Move::Cooperate;
        }

        const auto last = state.lastRound();
        const Move myLast = selfIndex == 0 ? last.first : last.second;
        const Move opponentLast = selfIndex == 0 ? last.second : last.first;

        if (m_remorseRemaining > 0) {
            if (opponentLast == Move::Cooperate) {
                m_remorseRemaining = std::max(m_remorseRemaining, 2);
            }
            else {
                m_remorseRemaining = 0;
                m_postRemorseBias = true;
            }
        }

        if (m_remorseRemaining == 0 && myLast == Move::Defect && opponentLast == Move::Cooperate) {
            m_remorseRemaining = 2;
            m_postRemorseBias = false;
        }

        if (m_remorseRemaining > 0) {
            const Move choice = rng.nextBool(0.8) ? Move::Cooperate : Move::Defect;
            m_remorseRemaining = std::max(0, m_remorseRemaining - 1);
            return choice;
        }

        if (m_postRemorseBias) {
            m_postRemorseBias = false;
            return rng.nextBool(0.6) ? Move::Cooperate : Move::Defect;
        }

        return opponentLast;
    }
}