#pragma once
#include <chrono>

namespace ipd {
    class Timer {
    public:
        Timer();

        void reset();
        double elapsedSeconds() const;

    private:
        std::chrono::steady_clock::time_point m_start;
    };
}