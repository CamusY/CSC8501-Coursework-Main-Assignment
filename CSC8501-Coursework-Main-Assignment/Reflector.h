#pragma once

#include "Strategy.h"

namespace ipd {
    class Reflector : public Strategy {
    public:
        Reflector();

        std::string name() const override { return "Reflector"; }
        Move nextMove(const MatchState& state, int selfIndex, Random& rng) override;
        void reset() override;
		double complexity() const override { return 3.0; }

    private:
        double payoffFor(Move self, Move opponent) const;

        double m_trust;
        double m_mood;
        double m_expectedReward;

        static constexpr double s_eta = 0.1;
        static constexpr double s_alpha = 0.2;
        static constexpr double s_beta = 0.2;
        static constexpr double s_temptation = 5.0;
        static constexpr double s_reward = 3.0;
        static constexpr double s_punishment = 1.0;
        static constexpr double s_sucker = 0.0;
    };
}