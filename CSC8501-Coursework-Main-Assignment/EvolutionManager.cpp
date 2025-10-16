﻿#include "EvolutionManager.h"

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

/**
 * @file EvolutionManager.cpp
 * @brief 实现了迭代囚徒困境（IPD）策略的演化模拟功能。
 *
 * 该文件包含了运行一个世代模拟的核心逻辑，其中策略的种群根据其在锦标赛中的表现进行演化。
 * 整个过程包括适应度评估、选择、繁殖和变异。
 *
 * --- 核心组件概述 ---
 *
 * I. 参数与常量
 * - kEpsilon: 一个微小常量，用于防止计算中出现除以零或负权重的问题。
 * - makeEvaluationConfig: 创建一个专门用于评估单个世代收益的配置，该配置不执行任何演化步骤。
 *
 * II. 适应度（Fitness）计算
 * - penalisedFitness: 通过从策略的平均得分中减去其复杂性惩罚来计算最终的适应度。
 * - collectFitness: 将锦标赛的结果映射为一个适应度向量，其顺序与全局策略列表（config.strategyNames）保持一致。
 *
 * III. 初始化与统计构造
 * - initialCounts: 将初始种群数量尽可能均匀地分配给所有参与的策略。
 * - toOrderedCounts: 创建一个按策略名称排序的 (策略名, 数量) 列表。
 * - makeGenerationShare: 为单个世代生成统计对象，包括各种策略的具体数量和种群份额。
 * - shareForStrategy: 查询特定策略在当前种群中的份额。
 *
 * IV. 概率计算与离散分配（重采样）
 * - computeProbabilities: 根据适应度（fitness）和当前各种群数量，计算下一代中每个策略被选择的概率。
 * 该函数通过平移适应度值来处理负数，并为总权重为零等极端情况提供了回退机制（先退化为按数量加权，若仍为零则均匀分布）。
 * - allocateNextGeneration: 基于计算出的概率来分配下一代的种群。采用一种确定性方法，
 * 先对期望数量取整，然后根据小数部分的大小依次分配剩余名额，以确保种群总数保持不变。
 *
 * V. 变异机制
 * - mutateCounts: 实现种群变异。它将每个策略的一部分个体（由变异率决定）重新分配给其他策略。
 * 这些“迁移者”的去向是根据全局的选择概率来决定的。
 *
 * VI. CSV 输出
 * - writeEvolutionSharesCsv: 将每一代各种群份额的历史记录写入到一个 CSV 文件中，便于后续分析。
 *
 * VII. 主流程
 * - EvolutionManager::run: 作为入口函数，负责协调并执行整个演化过程，
 * 其核心循环包括：评估、计算适应度、计算概率、生成下一代、变异、记录。
 */


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

    // ---------------------- EvolutionManager Implementation ----------------------

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
