#include "TournamentManager.h"
#include <algorithm>
#include <iterator>
#include <map>
#include <numeric>
#include <optional>
#include <utility>
#include <vector>

#include "Match.h"
#include "Statistics.h"
#include "StrategyFactory.h"
#include "Random.h"

namespace ipd {
    namespace {
        struct StrategyAggregate {
            std::vector<double> scores;
            std::optional<double> complexity;
        };

        using MatchPair = std::pair<std::string, std::string>;

        std::vector<MatchPair> generateMatchPairs(const std::vector<std::string>& strategyNames) {
            std::vector<MatchPair> pairs;
            pairs.reserve(strategyNames.size() * strategyNames.size());
            std::for_each(strategyNames.begin(), strategyNames.end(), [&](const std::string& firstName) {
                std::transform(strategyNames.begin(), strategyNames.end(), std::back_inserter(pairs), [&](const std::string& secondName) {
                    return MatchPair{ firstName, secondName };
                });
            });
            return pairs;
        }

        void accumulateScore(StrategyAggregate& aggregate, double score, double complexity) {
            aggregate.scores.push_back(score);
            if (!aggregate.complexity.has_value()) {
                aggregate.complexity = complexity;
            }
        }

        std::vector<Result> buildResults(const std::map<std::string, StrategyAggregate>& aggregates) {
            std::vector<Result> results;
            results.reserve(aggregates.size());
            std::transform(aggregates.begin(), aggregates.end(), std::back_inserter(results), [](const auto& entry) {
                const auto& name = entry.first;
                const auto& aggregate = entry.second;
                const double mean = statistics::mean(aggregate.scores);
                const double variance = statistics::variance(aggregate.scores, mean);
                const auto [ciLow, ciHigh] = statistics::confidenceInterval95(aggregate.scores, mean);

                Result result;
                result.strategy = name;
                result.mean = mean;
                result.variance = variance;
                result.ciLow = ciLow;
                result.ciHigh = ciHigh;
                result.complexity = aggregate.complexity.value_or(0.0);
                result.samples = aggregate.scores.size();
                return result;
            });
            return results;
        }
    }

    std::vector<Result> TournamentManager::run(const Config& config) const {
        registerBuiltinStrategies();

        if (config.repeats <= 0 || config.rounds <= 0) {
            return {};
        }

        StrategyFactory& factory = StrategyFactory::instance();

        Random rng;
        if (config.useSeed) {
            rng.reseed(config.seed);
        }

        std::map<std::string, StrategyAggregate> aggregates;

        Match match(config.payoff, config.noise);

        const auto matchPairs = generateMatchPairs(config.strategyNames);
        if (matchPairs.empty()) {
            return {};
        }

        const auto rounds = static_cast<double>(config.rounds);
        auto playMatch = [&](const MatchPair& pair) {
            StrategyPtr first = factory.create(pair.first);
            StrategyPtr second = factory.create(pair.second);

            const MatchReport report = match.play(*first, *second, config.rounds, rng);
            const double averageFirst = report.scoreFirst / rounds;
            const double averageSecond = report.scoreSecond / rounds;

            accumulateScore(aggregates[pair.first], averageFirst, first->complexity());
            accumulateScore(aggregates[pair.second], averageSecond, second->complexity());
        };

        for (int repeat = 0; repeat < config.repeats; ++repeat) {
            std::for_each(matchPairs.begin(), matchPairs.end(), playMatch);
        }

        return buildResults(aggregates);
    }
}
