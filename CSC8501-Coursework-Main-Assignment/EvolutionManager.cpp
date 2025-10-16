#include "EvolutionManager.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <map>
#include <numeric>
#include <stdexcept>
#include <vector>

#include "TournamentManager.h"
#include "StrategyFactory.h"

namespace ipd {
    namespace {

        constexpr double kEpsilon = 1e-12;

        Config makeEvaluationConfig(const Config& baseConfig) {
            Config evaluation = baseConfig;
            evaluation.generations = 0;
            evaluation.populationSize = 0;
            evaluation.mutationRate = 0.0;
            evaluation.evolve = false;
            evaluation.complexityPenalty = baseConfig.complexityPenalty;
            return evaluation;
        }

        inline double penalisedFitness(const Result& r, double penalty) {
            return r.netMean - penalty * r.complexity;
        }

        std::vector<double> collectFitness(
            const std::vector<std::string>& names,
            const std::vector<Result>& results,
            double penalty) {
            std::map<std::string, double> byName;
            for (const auto& r : results)
                byName[r.strategy] = penalisedFitness(r, penalty);

            std::vector<double> fitness(names.size(), 0.0);
            for (std::size_t i = 0; i < names.size(); ++i) {
                if (auto it = byName.find(names[i]); it != byName.end())
                    fitness[i] = it->second;
            }
            return fitness;
        }

        std::vector<int> initialCounts(const std::vector<std::string>& names, int population) {
            const int n = static_cast<int>(names.size());
            std::vector<int> counts(n, 0);
            if (n == 0 || population <= 0) return counts;
            const int base = population / n;
            int rem = population % n;
            for (int i = 0; i < n; ++i) {
                counts[i] = base + (rem > 0 ? 1 : 0);
                if (rem > 0) --rem;
            }
            return counts;
        }

        std::vector<std::pair<std::string, int>> toOrderedCounts(
            const std::vector<std::string>& names,
            const std::vector<int>& counts)
        {
            std::vector<std::pair<std::string, int>> ordered;
            for (std::size_t i = 0; i < names.size(); ++i)
                ordered.emplace_back(names[i], i < counts.size() ? counts[i] : 0);
            std::sort(ordered.begin(), ordered.end(),
                [](auto& a, auto& b) { return a.first < b.first; });
            return ordered;
        }

        GenerationShare makeGenerationShare(
            int generation,
            const std::vector<std::string>& names,
            const std::vector<int>& counts,
            int population)
        {
            GenerationShare g;
            g.generation = generation;
            g.counts = toOrderedCounts(names, counts);
            g.shares.reserve(g.counts.size());
            const double total = static_cast<double>(std::max(population, 0));
            for (const auto& [name, cnt] : g.counts) {
                const double ratio = (total > 0.0)
                    ? static_cast<double>(cnt) / total
                    : 0.0;
                g.shares.emplace_back(name, ratio);
            }
            return g;
        }

        double shareForStrategy(
            const std::vector<std::string>& names,
            const std::vector<int>& counts,
            int population,
            const std::string& strategy)
        {
            if (population <= 0) return 0.0;
            for (std::size_t i = 0; i < names.size() && i < counts.size(); ++i)
                if (names[i] == strategy)
                    return static_cast<double>(counts[i]) / static_cast<double>(population);
            return 0.0;
        }

        std::vector<double> computeProbabilities(
            const std::vector<int>& counts,
            const std::vector<double>& fitness)
        {
            const std::size_t n = counts.size();
            if (n == 0) return {};

            double minFit = std::numeric_limits<double>::infinity();
            for (double v : fitness) minFit = std::min(minFit, v);

            std::vector<double> weights(n, 0.0);
            double total = 0.0;
            for (std::size_t i = 0; i < n; ++i) {
                const double shifted = std::max((i < fitness.size() ? fitness[i] : 0.0) - minFit + kEpsilon, kEpsilon);
                const double w = shifted * static_cast<double>(std::max(counts[i], 0));
                weights[i] = w;
                total += w;
            }

            if (total <= 0.0) {
                for (std::size_t i = 0; i < n; ++i)
                    weights[i] = static_cast<double>(std::max(counts[i], 0));
                total = std::accumulate(weights.begin(), weights.end(), 0.0);
                if (total <= 0.0)
                    return std::vector<double>(n, 1.0 / static_cast<double>(n));
            }

            std::vector<double> p(n, 0.0);
            for (std::size_t i = 0; i < n; ++i) p[i] = weights[i] / total;
            return p;
        }
    }   

    EvolutionManager::EvolutionManager() : m_random() {}

    std::vector<int> EvolutionManager::sampleNextGeneration(
        const std::vector<double>& probabilities, int population)
    {
        const std::size_t n = probabilities.size();
        std::vector<int> counts(n, 0);
        if (n == 0 || population <= 0) return counts;

        std::discrete_distribution<int> dist(probabilities.begin(), probabilities.end());
        for (int i = 0; i < population; ++i) {
            int idx = dist(m_random.engine());
            ++counts[idx];
        }
        return counts;
    }

    void EvolutionManager::mutateCounts(
        std::vector<int>& counts,
        double mutationRate,
        const std::vector<double>& probabilities)
    {
		(void)probabilities;// currently unused
        const std::size_t n = counts.size();
        if (n <= 1 || mutationRate <= 0.0) return;

        for (std::size_t i = 0; i < n; ++i) {
            int c = counts[i];
            if (c <= 0) continue;
            int m = static_cast<int>(std::round(static_cast<double>(c) * mutationRate));
            for (int k = 0; k < m; ++k) {
                int target;
                do { target = m_random.nextInt(0, (int)n - 1); } while (target == (int)i);
                --counts[i];
                ++counts[target];
            }
        }
    }

    EvolutionOutcome EvolutionManager::run(const Config& config) {
        EvolutionOutcome out;
        if (!config.evolve && config.generations <= 0) return out;

        registerBuiltinStrategies();
        const std::size_t n = config.strategyNames.size();
        if (n == 0) return out;

        const int population = std::max(config.populationSize, 0);
        std::vector<int> counts = initialCounts(config.strategyNames, population);

        TournamentManager tm;
        std::vector<Result> lastFitness;
        const Config evalCfg = makeEvaluationConfig(config);

        out.history.push_back(makeGenerationShare(0, config.strategyNames, counts, population));

        for (int gen = 0; gen < config.generations; ++gen) {
            lastFitness = tm.run(evalCfg);

            const auto fitness = collectFitness(config.strategyNames, lastFitness, config.complexityPenalty);
            const auto probs = computeProbabilities(counts, fitness);

            std::vector<int> nextCounts = sampleNextGeneration(probs, population);
            mutateCounts(nextCounts, config.mutationRate, probs);

            counts = std::move(nextCounts);
            out.history.push_back(makeGenerationShare(gen + 1, config.strategyNames, counts, population));
        }

        if (lastFitness.empty()) lastFitness = tm.run(evalCfg);
        for (auto& r : lastFitness)
            r.extra = shareForStrategy(config.strategyNames, counts, population, r.strategy);

        out.results = std::move(lastFitness);
        return out;
    }

    void writeEvolutionSharesCsv(const Config& config, const std::vector<GenerationShare>& history) {
        if (history.empty()) return;

        namespace fs = std::filesystem;
        fs::path out = config.outputFile.empty()
            ? fs::path("evolution_shares.csv")
            : fs::path(config.outputFile).parent_path() / "evolution_shares.csv";

        if (!out.parent_path().empty() && !fs::exists(out.parent_path()))
            fs::create_directories(out.parent_path());

        std::ofstream os(out, std::ios::trunc);
        if (!os)
            throw std::runtime_error("Unable to write evolution share log: " + out.string());

        os << "generation,strategy,count,share\n";
        for (const auto& g : history) {
            std::map<std::string, int> countLookup;
            for (const auto& kv : g.counts)
                countLookup[kv.first] = kv.second;

            for (const auto& kv : g.shares) {
                os << g.generation << ',' << kv.first << ','
                    << countLookup[kv.first] << ','
                    << std::fixed << std::setprecision(6) << kv.second << '\n';
            }
        }
    }
}
