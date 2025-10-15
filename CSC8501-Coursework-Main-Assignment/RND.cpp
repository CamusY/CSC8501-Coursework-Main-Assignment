#include "RND.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>

namespace ipd {
    RND::RND(double probability)
        : m_probability(probability) {
        m_probability = std::clamp(m_probability, 0.0, 1.0);
        if (std::abs(m_probability - 0.5) < 1e-9) {
            m_name = "RND";
            return;
        }
        std::ostringstream builder;
        builder << "RND(" << std::fixed << std::setprecision(3) << m_probability << ")";
        m_name = builder.str();
    }

    std::string RND::name() const {
        return m_name;
    }

    Move RND::nextMove(const MatchState&, int, Random& rng) {
        return rng.nextBool(m_probability) ? Move::Cooperate : Move::Defect;
    }

    double RND::complexity() const {
        return 1.5;
    }
}