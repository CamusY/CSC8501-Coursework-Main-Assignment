#pragma once
#include <random>
namespace ipd {
    class Random {
    public:
        Random();
        explicit Random(unsigned int seed);

        void reseed(unsigned int seed);

        double nextDouble(double minInclusive = 0.0, double maxExclusive = 1.0);
        bool nextBool(double probability = 0.5);
        int nextInt(int minInclusive, int maxInclusive);

        std::mt19937& engine();

    private:
        std::mt19937 m_engine;
    };
}