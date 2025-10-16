#pragma once

#include "Strategy.h"

namespace ipd {
    class PROBER : public Strategy {
    public:
        PROBER();

        std::string name() const override;
        Move nextMove(const MatchState& state, int selfIndex, Random& rng) override;
        void reset() override;
        int complexity() const override;

    private:
        int m_round;
        bool m_exploit;
    };
}