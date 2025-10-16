#pragma once

#include "Strategy.h"

namespace ipd {
    class RND : public Strategy {
    public:
        explicit RND(double probability = 0.5);
        std::string name() const override;
        Move nextMove(const MatchState& state, int selfIndex, Random& rng) override;
        int complexity() const override;

    private:
        double m_probability;
        std::string m_name;
    };
}