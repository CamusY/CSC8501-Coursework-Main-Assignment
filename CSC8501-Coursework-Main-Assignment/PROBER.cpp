#include "PROBER.h"
namespace ipd {
    PROBER::PROBER()
        : m_round(0), m_exploit(false) {
    }

    std::string PROBER::name() const {
        return "PROBER";
    }

    Move PROBER::nextMove(const MatchState& state, int selfIndex, Random&) {
        if (m_round == 0){
            ++m_round;
            return Move::Defect;
        }
        if (m_round == 1) {
            ++m_round;
            return Move::Cooperate;
        }
        if (m_round == 2) {
            ++m_round;
            if (state.hasHistory()) {
                const auto& history = state.history();
                if (history.size() >= 2) {
                    const Move opp0 = selfIndex == 0 ? history[0].second : history[0].first;
                    const Move opp1 = selfIndex == 0 ? history[1].second : history[1].first;
                    m_exploit = (opp0 == Move::Cooperate && opp1 == Move::Cooperate);
                }
            }
            return Move::Cooperate;
        }

        if (m_exploit) {
            return Move::Defect;
        }

        if (!state.hasHistory()) {
            return Move::Cooperate;
        }

        return state.lastOpponentMove(selfIndex);
    }

    void PROBER::reset() {
        m_round = 0;
        m_exploit = false;
    }

    int PROBER::complexity() const {
        return 3;
    }
}
