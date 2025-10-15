#pragma once

#include "MatchState.h"
#include "Payoff.h"
#include "Strategy.h"
#include "Random.h"

namespace ipd {
    struct MatchReport {
        double scoreFirst = 0.0;
        double scoreSecond = 0.0;
        MatchState state;
    };

    class Match {
    public:
        Match(const Payoff& payoff, double noiseProbability);

        MatchReport play(Strategy& first, Strategy& second, int rounds, Random& rng);

    private:
        double evaluateRound(Move first, Move second) const;

        Payoff m_payoff;
        double m_noise;
    };
}