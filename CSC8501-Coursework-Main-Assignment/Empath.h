#pragma once

#include <cstddef>

#include "Strategy.h"

namespace ipd {
    class Empath : public Strategy {
    public:
        Empath();

        std::string name() const override { return "Empath"; }
        Move nextMove(const MatchState& state, int selfIndex, Random& rng) override;
        void reset() override;
		double complexity() const override { return 3.0; }

    private:
        int m_remorseRemaining;
        bool m_postRemorseBias;
    };
}