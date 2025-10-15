#include "Match.h"
namespace ipd {
    Match::Match(const Payoff& payoff, double epsilon)
        : m_payoff(payoff), m_epsilon(epsilon) {
    }

    MatchReport Match::play(Strategy& first, Strategy& second, int rounds, Random& rng) {
        MatchReport report;
        report.state.reset();
        first.reset();
        second.reset();

        for (int round = 0; round < rounds; ++round) {
            Move moveFirst = first.nextMove(report.state, 0, rng);
            Move moveSecond = second.nextMove(report.state, 1, rng);

            if (m_epsilon > 0.0) {
                if (rng.nextBool(m_epsilon)) {
                    moveFirst = flip(moveFirst);
                }
                if (rng.nextBool(m_epsilon)) {
                    moveSecond = flip(moveSecond);
                }
            }

            report.state.recordRound(moveFirst, moveSecond);

            report.scoreFirst += evaluateRound(moveFirst, moveSecond);
            report.scoreSecond += evaluateRound(moveSecond, moveFirst);
        }

        first.onMatchEnd(report.state, 0);
        second.onMatchEnd(report.state, 1);

        return report;
    }

    double Match::evaluateRound(Move first, Move second) const {
        if (first == Move::Cooperate && second == Move::Cooperate) {
            return m_payoff.R;
        }
        if (first == Move::Cooperate && second == Move::Defect) {
            return m_payoff.S;
        }
        if (first == Move::Defect && second == Move::Cooperate) {
            return m_payoff.T;
        }
        return m_payoff.P;
    }
}