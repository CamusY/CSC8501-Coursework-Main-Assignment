#pragma once

#include <string>
#include <utility>
#include <vector>

#include "Config.h"
#include "Result.h"

namespace ipd {
    struct GenerationShare {
        int generation = 0;
        std::vector<std::pair<std::string, double>> shares;
    };

    struct EvolutionOutcome {
        std::vector<Result> results;
        std::vector<GenerationShare> history;
    };

    class EvolutionManager {
    public:
        EvolutionManager() = default;

        EvolutionOutcome run(const Config& config) const;
    };

    void writeEvolutionSharesCsv(const Config& config, const std::vector<GenerationShare>& history);
}