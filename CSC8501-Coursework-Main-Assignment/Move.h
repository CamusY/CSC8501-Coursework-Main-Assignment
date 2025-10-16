#pragma once
#include <string>
#include <ostream>
namespace ipd {
    enum class Move {
        Cooperate,
        Defect
    };
    std::string toString(Move move);
    Move flip(Move move);
	std::ostream& operator<<(std::ostream& os, Move move);
}