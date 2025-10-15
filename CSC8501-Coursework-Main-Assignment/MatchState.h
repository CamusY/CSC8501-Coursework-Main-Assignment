#pragma once

#include <cstddef>
#include <optional>
#include <utility>
#include <vector>

#include "Move.h"

namespace ipd {
    class MatchState {
    public:
        struct Round {
            Move first;
            Move second;
        };

        MatchState();

        void reset();
        void recordRound(Move first, Move second);

        std::size_t roundsPlayed() const;
        bool hasHistory() const;
        Round lastRound() const;
        Move lastMove(int playerIndex) const;
        Move lastOpponentMove(int playerIndex) const;
        std::size_t defections(int playerIndex) const;

        const std::vector<Round>& history() const;

    private:
        std::vector<Round> m_history;
    };
}