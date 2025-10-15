#include "EvolutionManager.h"

#include <algorithm>
#include <map>
#include <numeric>

#include "TournamentManager.h"
#include "StrategyFactory.h"

namespace ipd {
    namespace {
        Config makeEvaluationConfig(const Config& baseConfig) {
            Config evaluationConfig = baseConfig;
            evaluationConfig.generations = 0;
            evaluationConfig.populationSize = 0;
            evaluationConfig.mutationRate = 0.0;
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
    }

    std::vector<Result> EvolutionManager::run(const Config& config) const {
        if (config.generations <= 0) {
            return {};
        }

        registerBuiltinStrategies();

        const std::size_t strategyCount = config.strategyNames.size();
        if (strategyCount == 0) {
            return {};
        }

        std::map<std::string, double> proportions;
        const double initialShare = 1.0 / static_cast<double>(strategyCount);
        std::for_each(config.strategyNames.begin(), config.strategyNames.end(), [&](const std::string& name) {
            proportions[name] = initialShare;
        });

        TournamentManager tournament;
        std::vector<Result> lastFitness;
        const Config evaluationConfig = makeEvaluationConfig(config);

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

            std::map<std::string, double> nextProportions;
            if (totalWeightedFitness <= 0.0) {
                nextProportions = proportions;
            }
            else {
                std::for_each(config.strategyNames.begin(), config.strategyNames.end(), [&](const std::string& name) {
                    const double fitness = clampedFitness(strategyFitness(fitnessMap, name));
                    const double share = (proportions.at(name) * fitness) / totalWeightedFitness;
                    const double mutatedShare = (1.0 - config.mutationRate) * share + (config.mutationRate / static_cast<double>(strategyCount));
                    nextProportions[name] = mutatedShare;
                });
            }

            proportions = std::move(nextProportions);
        }

        if (lastFitness.empty()) {
            TournamentManager tournamentManager;
            lastFitness = tournamentManager.run(evaluationConfig);
        }

        std::for_each(lastFitness.begin(), lastFitness.end(), [&](Result& result) {
            result.extra = strategyFitness(proportions, result.strategy);
        });

        return lastFitness;
    }
}