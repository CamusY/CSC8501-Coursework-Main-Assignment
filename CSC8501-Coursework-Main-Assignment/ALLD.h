#pragma once

#include "Strategy.h"
namespace ipd {
    class ALLD : public Strategy {
    public:
        std::string name() const override;
        Move nextMove(const MatchState& state, int selfIndex, Random& rng) override;
        int complexity() const override;
    };
}