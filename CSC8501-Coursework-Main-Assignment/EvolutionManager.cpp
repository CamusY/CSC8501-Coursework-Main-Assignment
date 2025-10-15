#include "EvolutionManager.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <map>
#include <numeric>
#include <stdexcept>

#include "TournamentManager.h"
#include "StrategyFactory.h"

namespace ipd {
    namespace {
        Config makeEvaluationConfig(const Config& baseConfig) {
            Config evaluationConfig = baseConfig;
            evaluationConfig.generations = 0;
            evaluationConfig.populationSize = 0;
            evaluationConfig.mutationRate = 0.0;
            evaluationConfig.evolve = false;
            evaluationConfig.complexityPenalty = baseConfig.complexityPenalty;
            return evaluationConfig;
        }

        double penalisedFitness(const Result& result, double penalty) {
            return result.mean - penalty * result.complexity;
        }

        double clampedFitness(double value) {
            return std::max(0.0, value);
        }

        double strategyFitness(const std::map<std::string, double>& fitnessMap, const std::string& strategy) {
            const auto iterator = fitnessMap.find(strategy);
            if (iterator == fitnessMap.end()) {
                return 0.0;
            }
            return iterator->second;
        }

        std::vector<std::pair<std::string, double>> toOrderedShares(const std::map<std::string, double>& proportions) {
            std::vector<std::pair<std::string, double>> ordered;
            ordered.reserve(proportions.size());
            std::transform(proportions.begin(), proportions.end(), std::back_inserter(ordered), [](const auto& entry) {
                return entry;
                });
            std::sort(ordered.begin(), ordered.end(), [](const auto& lhs, const auto& rhs) {
                return lhs.first < rhs.first;
                });
            return ordered;
        }
    }

    EvolutionOutcome EvolutionManager::run(const Config& config) const {
        EvolutionOutcome outcome;
        if (!config.evolve && config.generations <= 0) {
            return outcome;
        }

        registerBuiltinStrategies();

        const std::size_t strategyCount = config.strategyNames.size();
        if (strategyCount == 0) {
            return outcome;
        }

        std::map<std::string, double> proportions;
        const double initialShare = 1.0 / static_cast<double>(strategyCount);
        std::for_each(config.strategyNames.begin(), config.strategyNames.end(), [&](const std::string& name) {
            proportions[name] = initialShare;
            });

        TournamentManager tournament;
        std::vector<Result> lastFitness;
        const Config evaluationConfig = makeEvaluationConfig(config);

        outcome.history.push_back(GenerationShare{ 0, toOrderedShares(proportions) });

        for (int generation = 0; generation < config.generations; ++generation) {
            lastFitness = tournament.run(evaluationConfig);
            std::map<std::string, double> fitnessMap;
            std::for_each(lastFitness.begin(), lastFitness.end(), [&](const Result& result) {
                fitnessMap[result.strategy] = penalisedFitness(result, config.complexityPenalty);
                });

            const double totalWeightedFitness = std::accumulate(config.strategyNames.begin(), config.strategyNames.end(), 0.0,
                [&](double total, const std::string& name) {
                    const double fitness = clampedFitness(strategyFitness(fitnessMap, name));
                    return total + proportions.at(name) * fitness;
                });

            std::map<std::string, double> nextProportions = proportions;
            if (totalWeightedFitness > 0.0) {
                std::for_each(config.strategyNames.begin(), config.strategyNames.end(), [&](const std::string& name) {
                    const double fitness = clampedFitness(strategyFitness(fitnessMap, name));
                    const double share = (proportions.at(name) * fitness) / totalWeightedFitness;
                    const double mutatedShare = (1.0 - config.mutationRate) * share + (config.mutationRate / static_cast<double>(strategyCount));
                    nextProportions[name] = mutatedShare;
                    });
            }

            proportions = std::move(nextProportions);
            outcome.history.push_back(GenerationShare{ generation + 1, toOrderedShares(proportions) });
        }

        if (lastFitness.empty()) {
            lastFitness = tournament.run(evaluationConfig);
        }

        std::for_each(lastFitness.begin(), lastFitness.end(), [&](Result& result) {
            result.extra = strategyFitness(proportions, result.strategy);
            });

        outcome.results = std::move(lastFitness);
        return outcome;
    }

    void writeEvolutionSharesCsv(const Config& config, const std::vector<GenerationShare>& history) {
        if (history.empty()) {
            return;
        }
        namespace fs = std::filesystem;
        fs::path outputPath = config.outputFile.empty()
            ? fs::path("evolution_shares.csv")
            : fs::path(config.outputFile).parent_path() / "evolution_shares.csv";
        if (!outputPath.parent_path().empty() && !fs::exists(outputPath.parent_path())) {
            fs::create_directories(outputPath.parent_path());
        }

        std::ofstream stream(outputPath, std::ios::trunc);
        if (!stream) {
            throw std::runtime_error("Unable to write evolution share log: " + outputPath.string());
        }
        stream << "generation,strategy,share\n";
        for (const auto& entry : history) {
            for (const auto& [strategy, share] : entry.shares) {
                stream << entry.generation << ',' << strategy << ',' << std::fixed << std::setprecision(6) << share << '\n';
            }
        }
    }
}