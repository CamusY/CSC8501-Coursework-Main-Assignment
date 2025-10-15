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

    private:
        int m_remorseRemaining;
        bool m_postRemorseBias;
    };
}