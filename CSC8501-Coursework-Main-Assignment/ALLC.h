#pragma once

#include "Strategy.h"

namespace ipd {
    class ALLC : public Strategy {
    public:
        std::string name() const override;
        Move nextMove(const MatchState& state, int selfIndex, Random& rng) override;
        double complexity() const override;
    };
}