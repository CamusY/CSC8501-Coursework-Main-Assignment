#pragma once
#include <string>
namespace ipd {
    enum class Move {
        Cooperate,
        Defect
    };
    std::string toString(Move move);
    Move flip(Move move);
}