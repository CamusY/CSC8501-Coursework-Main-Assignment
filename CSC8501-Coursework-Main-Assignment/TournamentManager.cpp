#include "TournamentManager.h"
#include <algorithm>
#include <cmath>
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
        struct MatchMetrics {
            double cooperations = 0.0;
            double rounds = 0.0;
            double firstDefection = 0.0;
            double echoLengthSum = 0.0;
            std::size_t echoSamples = 0;
        };

        struct StrategyAggregate {
            std::vector<double> scores;
            std::optional<double> complexity;
            double cooperationTotal = 0.0;
            double roundTotal = 0.0;
            double firstDefectionTotal = 0.0;
            std::size_t firstDefectionSamples = 0;
            double echoLengthTotal = 0.0;
            std::size_t echoLengthSamples = 0;
        };

        using MatchPair = std::pair<std::string, std::string>;

        std::vector<MatchPair> generateMatchPairs(const std::vector<std::string>& strategyNames) {
            std::vector<MatchPair> pairs;
            pairs.reserve(strategyNames.size() * strategyNames.size());
            for (const auto& firstName : strategyNames) {
                std::transform(strategyNames.begin(), strategyNames.end(), std::back_inserter(pairs), [&](const std::string& secondName) {
                    return MatchPair{ firstName, secondName };
                    });
            }
            return pairs;
        }

        MatchMetrics computeMetrics(const MatchState& state, int playerIndex, int totalRounds) {
            MatchMetrics metrics;

            const auto& history = state.history();
            metrics.rounds = static_cast<double>(history.size());

            bool hasDefection = false;
            bool lastMutualCoop = true;
            bool inEcho = false;
            std::size_t currentEcho = 0;

            for (std::size_t index = 0; index < history.size(); ++index) {
                const auto& round = history[index];
                const Move self = playerIndex == 0 ? round.first : round.second;
                const Move opponent = playerIndex == 0 ? round.second : round.first;

                if (self == Move::Cooperate) {
                    metrics.cooperations += 1.0;
                }

                if (!hasDefection && self == Move::Defect) {
                    metrics.firstDefection = static_cast<double>(index) + 1.0;
                    hasDefection = true;
                }

                const bool mutualCooperate = (self == Move::Cooperate && opponent == Move::Cooperate);

                if (inEcho) {
                    ++currentEcho;
                }
                else if (lastMutualCoop && !mutualCooperate) {
                    inEcho = true;
                    currentEcho = 1;
                }

                if (mutualCooperate) {
                    if (inEcho) {
                        metrics.echoLengthSum += static_cast<double>(currentEcho);
                        metrics.echoSamples += 1;
                        inEcho = false;
                        currentEcho = 0;
                    }
                    lastMutualCoop = true;
                }
                else {
                    lastMutualCoop = false;
                }
            }

            if (!hasDefection) {
                metrics.firstDefection = static_cast<double>(totalRounds) + 1.0;
            }

            if (inEcho && currentEcho > 0) {
                metrics.echoLengthSum += static_cast<double>(currentEcho);
                metrics.echoSamples += 1;
            }

            return metrics;
        }

        void accumulateScore(StrategyAggregate& aggregate, double score, double complexity, const MatchMetrics& metrics) {
            aggregate.scores.push_back(score);
            if (!aggregate.complexity.has_value()) {
                aggregate.complexity = complexity;
            }
            aggregate.cooperationTotal += metrics.cooperations;
            aggregate.roundTotal += metrics.rounds;
            aggregate.firstDefectionTotal += metrics.firstDefection;
            aggregate.firstDefectionSamples += 1;
            aggregate.echoLengthTotal += metrics.echoLengthSum;
            aggregate.echoLengthSamples += metrics.echoSamples;
        }

        std::vector<Result> buildResults(const std::map<std::string, StrategyAggregate>& aggregates) {
            std::vector<Result> results;
            results.reserve(aggregates.size());
            std::transform(aggregates.begin(), aggregates.end(), std::back_inserter(results), [](const auto& entry) {
                const auto& name = entry.first;
                const auto& aggregate = entry.second;
                const double mean = statistics::mean(aggregate.scores);
                const double variance = statistics::variance(aggregate.scores, mean);
                const double stdev = std::sqrt(variance);
                const auto [ciLow, ciHigh] = statistics::confidenceInterval95(aggregate.scores, mean);

                Result result;
                result.strategy = name;
                result.mean = mean;
                result.variance = variance;
                result.stdev = stdev;
                result.ciLow = ciLow;
                result.ciHigh = ciHigh;
                result.coopRate = aggregate.roundTotal > 0.0 ? aggregate.cooperationTotal / aggregate.roundTotal : 0.0;
                result.firstDefection = aggregate.firstDefectionSamples > 0
                    ? aggregate.firstDefectionTotal / static_cast<double>(aggregate.firstDefectionSamples)
                    : 0.0;
                result.echoLength = aggregate.echoLengthSamples > 0
                    ? aggregate.echoLengthTotal / static_cast<double>(aggregate.echoLengthSamples)
                    : 0.0;
                result.complexity = aggregate.complexity.value_or(0.0);
                result.samples = aggregate.scores.size();
                return result;
                });
            std::sort(results.begin(), results.end(), [](const Result& lhs, const Result& rhs) {
                return lhs.mean > rhs.mean;
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

        Match match(config.payoffs, config.epsilon);

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

            const MatchMetrics firstMetrics = computeMetrics(report.state, 0, config.rounds);
            const MatchMetrics secondMetrics = computeMetrics(report.state, 1, config.rounds);

            accumulateScore(aggregates[pair.first], averageFirst, first->complexity(), firstMetrics);
            accumulateScore(aggregates[pair.second], averageSecond, second->complexity(), secondMetrics);
        };

        for (int repeat = 0; repeat < config.repeats; ++repeat) {
            std::for_each(matchPairs.begin(), matchPairs.end(), playMatch);
        }

        return buildResults(aggregates);
    }
}
