#include "Random.h"

#include <chrono>

namespace ipd {
    namespace {
        std::mt19937 makeRandomEngine() {
            const auto now = std::chrono::high_resolution_clock::now().time_since_epoch();
            return std::mt19937(static_cast<unsigned int>(now.count()));
        }
    }

    Random::Random()
        : m_engine(makeRandomEngine()) {}

    Random::Random(unsigned int seed)
        : m_engine(seed) {}

    void Random::reseed(unsigned int seed) {
        m_engine.seed(seed);
}

    double Random::nextDouble(double minInclusive, double maxExclusive) {
        std::uniform_real_distribution<double> distribution(minInclusive, maxExclusive);
        return distribution(m_engine);
    }

    bool Random::nextBool(double probability) {
        std::bernoulli_distribution distribution(probability);
        return distribution(m_engine);
    }

    int Random::nextInt(int minInclusive, int maxInclusive) {
        std::uniform_int_distribution<int> distribution(minInclusive, maxInclusive);
        return distribution(m_engine);
    }

    std::mt19937& Random::engine() {
        return m_engine;
    }
}
