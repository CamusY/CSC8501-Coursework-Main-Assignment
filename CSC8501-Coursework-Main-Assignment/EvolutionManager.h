#pragma once

#include <string>
#include <vector>

#include "Config.h"
#include "Result.h"

namespace ipd {
    class EvolutionManager {
    public:
        EvolutionManager() = default;

        std::vector<Result> run(const Config& config) const;
    };
}