#pragma once

#include "Strategy.h"

namespace ipd{
    class PAVLOV : public Strategy {
    public:
        std::string name() const override;
        Move nextMove(const MatchState& state, int selfIndex, Random& rng) override;
        void reset() override;
        int complexity() const override;

    private:
        Move m_lastMove = Move::Cooperate;
    };
}