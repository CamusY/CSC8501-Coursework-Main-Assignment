#include "Reflector.h"

#include <algorithm>

namespace ipd {
    Reflector::Reflector()
        : m_trust(0.5)
        , m_mood(0.0)
        , m_expectedReward(s_reward) {
    }

    void Reflector::reset() {
        m_trust = 0.5;
        m_mood = 0.0;
        m_expectedReward = s_reward;
    }

    double Reflector::payoffFor(Move self, Move opponent) const {
        if (self == Move::Cooperate && opponent == Move::Cooperate) {
            return s_reward;
        }
        if (self == Move::Cooperate && opponent == Move::Defect) {
            return s_sucker;
        }
        if (self == Move::Defect && opponent == Move::Cooperate) {
            return s_temptation;
        }
        return s_punishment;
    }

    Move Reflector::nextMove(const MatchState& state, int selfIndex, Random& rng) {
        if (state.hasHistory()) {
            const auto last = state.lastRound();
            const Move myLast = selfIndex == 0 ? last.first : last.second;
            const Move opponentLast = selfIndex == 0 ? last.second : last.first;

            const double lastPayoff = payoffFor(myLast, opponentLast);

            if (lastPayoff > m_expectedReward) {
                m_mood = std::clamp(m_mood + s_alpha, -1.0, 1.0);
            }
            else {
                m_mood = std::clamp(m_mood - s_beta, -1.0, 1.0);
            }

            double learningRate = s_eta * (1.0 + m_mood);
            if (learningRate < 0.0) {
                learningRate = 0.0;
            }

            if (opponentLast == Move::Cooperate) {
                m_trust += learningRate * (1.0 - m_trust);
            }
            else {
                m_trust -= learningRate * m_trust;
            }
            m_trust = std::clamp(m_trust, 0.0, 1.0);

            m_expectedReward += s_eta * (lastPayoff - m_expectedReward);
        }
        else {
            reset();
        }

        return rng.nextBool(m_trust) ? Move::Cooperate : Move::Defect;
    }
}