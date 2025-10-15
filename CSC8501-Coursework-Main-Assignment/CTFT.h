#pragma once

#include "Strategy.h"

namespace ipd {
    class CTFT : public Strategy {
    public:
        CTFT();

        std::string name() const override;
        Move nextMove(const MatchState& state, int selfIndex, Random& rng) override;
        void reset() override;
        double complexity() const override;

    private:
        bool m_contrite;
    };
}