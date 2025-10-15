#include "MatchState.h"

#include <stdexcept>

namespace ipd {
    MatchState::MatchState() = default;

    void MatchState::reset() {
        m_history.clear();
    }

    void MatchState::recordRound(Move first, Move second) {
        m_history.push_back({ first, second });
    }

    std::size_t MatchState::roundsPlayed() const {
        return m_history.size();
    }

    bool MatchState::hasHistory() const {
        return !m_history.empty();
    }

    MatchState::Round MatchState::lastRound() const {
        if (m_history.empty()) {
            throw std::runtime_error("No rounds have been played yet");
        }
        return m_history.back();
    }

    Move MatchState::lastMove(int playerIndex) const {
        if (m_history.empty()) {
            throw std::runtime_error("No rounds have been played yet");
        }

        return playerIndex == 0 ? m_history.back().first : m_history.back().second;
    }

    Move MatchState::lastOpponentMove(int playerIndex) const {
        if (m_history.empty()) {
            throw std::runtime_error("No rounds have been played yet");
        }

        return playerIndex == 0 ? m_history.back().second : m_history.back().first;
    }

    std::size_t MatchState::defections(int playerIndex) const {
        std::size_t count = 0;
        for (const auto& round : m_history) {
            const auto move = playerIndex == 0 ? round.first : round.second;
            if (move == Move::Defect) {
                ++count;
            }
        }
        return count;
    }

    const std::vector<MatchState::Round>& MatchState::history() const {
        return m_history;
    }
}