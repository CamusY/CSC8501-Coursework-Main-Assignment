#pragma once

#include "Strategy.h"

namespace ipd {
    class GRIM : public Strategy {
    public:
        GRIM();

        std::string name() const override;
        Move nextMove(const MatchState& state, int selfIndex, Random& rng) override;
        void reset() override;
        double complexity() const override;

    private:
        bool m_triggered;
    };
}

