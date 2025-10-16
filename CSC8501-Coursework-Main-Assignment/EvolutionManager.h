#pragma once

#include <string>
#include <utility>
#include <vector>

#include "Config.h"
#include "Result.h"
#include "Random.h"

namespace ipd {

    struct GenerationShare {
        int generation = 0;
        std::vector<std::pair<std::string, double>> shares;
        std::vector<std::pair<std::string, int>> counts;
    };

    struct EvolutionOutcome {
        std::vector<Result> results;
        std::vector<GenerationShare> history;
    };

    class EvolutionManager {
    public:
        EvolutionManager();

        EvolutionOutcome run(const Config& config);

    private:
        Random m_random; 

        std::vector<int> sampleNextGeneration(
            const std::vector<double>& probabilities,
            int population);

        void mutateCounts(
            std::vector<int>& counts,
            double mutationRate,
            const std::vector<double>& probabilities);
    };

    void writeEvolutionSharesCsv(const Config& config, const std::vector<GenerationShare>& history);
}
