#include "Timer.h"

namespace ipd {
    Timer::Timer()
        : m_start(std::chrono::steady_clock::now()) {
    }

    void Timer::reset() {
        m_start = std::chrono::steady_clock::now();
    }

    double Timer::elapsedSeconds() const {
        const auto now = std::chrono::steady_clock::now();
        const std::chrono::duration<double> elapsed = now - m_start;
        return elapsed.count();
    }
}
