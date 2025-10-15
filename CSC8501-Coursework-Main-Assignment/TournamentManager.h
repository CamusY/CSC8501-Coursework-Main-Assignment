#pragma once

#include <map>
#include <string>
#include <vector>

#include "Config.h"
#include "Result.h"

namespace ipd {
    class TournamentManager {
    public:
        TournamentManager() = default;

        std::vector<Result> run(const Config& config) const;
    };
}