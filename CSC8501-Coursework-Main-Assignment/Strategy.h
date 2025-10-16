#pragma once

#include <memory>
#include <string>

#include "MatchState.h"
#include "Move.h"
#include "Random.h"

namespace ipd {
    class Strategy {
    public:
        virtual ~Strategy() = default;

        virtual std::string name() const = 0;
        virtual Move nextMove(const MatchState& state, int selfIndex, Random& rng) = 0;
        virtual void onMatchEnd(const MatchState& state, int selfIndex) { (void)state; (void)selfIndex; }
        virtual void reset() {}
        virtual int complexity() const { return 1; }
    };

    using StrategyPtr = std::unique_ptr<Strategy>;
}