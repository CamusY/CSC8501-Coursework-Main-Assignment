#include "EvolutionManager.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <map>
#include <numeric>
#include <stdexcept>
#include <utility>
#include <vector>

#include "TournamentManager.h"
#include "StrategyFactory.h"

/**
 * @file EvolutionManager.cpp
 * @brief ʵ���˵�����ͽ������IPD�����Ե��ݻ�ģ�⹦�ܡ�
 *
 * ���ļ�����������һ������ģ��ĺ����߼������в��Ե���Ⱥ�������ڽ������еı��ֽ����ݻ���
 * �������̰�����Ӧ��������ѡ�񡢷�ֳ�ͱ��졣
 *
 * --- ����������� ---
 *
 * I. �����볣��
 * - kEpsilon: һ��΢С���������ڷ�ֹ�����г��ֳ������Ȩ�ص����⡣
 * - makeEvaluationConfig: ����һ��ר��������������������������ã������ò�ִ���κ��ݻ����衣
 *
 * II. ��Ӧ�ȣ�Fitness������
 * - penalisedFitness: ͨ���Ӳ��Ե�ƽ���÷��м�ȥ�临���Գͷ����������յ���Ӧ�ȡ�
 * - collectFitness: ���������Ľ��ӳ��Ϊһ����Ӧ����������˳����ȫ�ֲ����б�config.strategyNames������һ�¡�
 *
 * III. ��ʼ����ͳ�ƹ���
 * - initialCounts: ����ʼ��Ⱥ���������ܾ��ȵط�������в���Ĳ��ԡ�
 * - toOrderedCounts: ����һ����������������� (������, ����) �б�
 * - makeGenerationShare: Ϊ������������ͳ�ƶ��󣬰������ֲ��Եľ�����������Ⱥ�ݶ
 * - shareForStrategy: ��ѯ�ض������ڵ�ǰ��Ⱥ�еķݶ
 *
 * IV. ���ʼ�������ɢ���䣨�ز�����
 * - computeProbabilities: ������Ӧ�ȣ�fitness���͵�ǰ����Ⱥ������������һ����ÿ�����Ա�ѡ��ĸ��ʡ�
 * �ú���ͨ��ƽ����Ӧ��ֵ������������Ϊ��Ȩ��Ϊ��ȼ�������ṩ�˻��˻��ƣ����˻�Ϊ��������Ȩ������Ϊ������ȷֲ�����
 * - allocateNextGeneration: ���ڼ�����ĸ�����������һ������Ⱥ������һ��ȷ���Է�����
 * �ȶ���������ȡ����Ȼ�����С�����ֵĴ�С���η���ʣ�������ȷ����Ⱥ�������ֲ��䡣
 *
 * V. �������
 * - mutateCounts: ʵ����Ⱥ���졣����ÿ�����Ե�һ���ָ��壨�ɱ����ʾ��������·�����������ԡ�
 * ��Щ��Ǩ���ߡ���ȥ���Ǹ���ȫ�ֵ�ѡ������������ġ�
 *
 * VI. CSV ���
 * - writeEvolutionSharesCsv: ��ÿһ������Ⱥ�ݶ����ʷ��¼д�뵽һ�� CSV �ļ��У����ں���������
 *
 * VII. ������
 * - EvolutionManager::run: ��Ϊ��ں���������Э����ִ�������ݻ����̣�
 * �����ѭ��������������������Ӧ�ȡ�������ʡ�������һ�������졢��¼��
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
            return r.mean - penalty * r.complexity;
        }

        std::vector<double> collectFitness(const std::vector<std::string>& names,
            const std::vector<Result>& results,
            double penalty) {
            std::map<std::string, double> byName;
            for (const auto& r : results) {
                byName[r.strategy] = penalisedFitness(r, penalty);
            }
            std::vector<double> fitness(names.size(), 0.0);
            for (std::size_t i = 0; i < names.size(); ++i) {
                auto it = byName.find(names[i]);
                if (it != byName.end()) fitness[i] = it->second;
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

        std::vector<std::pair<std::string, int>> toOrderedCounts(const std::vector<std::string>& names,
            const std::vector<int>& counts) {
            std::vector<std::pair<std::string, int>> ordered;
            ordered.reserve(names.size());
            for (std::size_t i = 0; i < names.size(); ++i) {
                const int c = (i < counts.size() ? counts[i] : 0);
                ordered.emplace_back(names[i], c);
            }
            std::sort(ordered.begin(), ordered.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
            return ordered;
        }

        GenerationShare makeGenerationShare(int generation,
            const std::vector<std::string>& names,
            const std::vector<int>& counts,
            int population) {
            GenerationShare g;
            g.generation = generation;
            g.counts = toOrderedCounts(names, counts);
            g.shares.reserve(g.counts.size());
            const double total = static_cast<double>(std::max(population, 0));
            for (const auto& [name, cnt] : g.counts) {
                const double ratio = (total > 0.0) ? (static_cast<double>(cnt) / total) : 0.0;
                g.shares.emplace_back(name, ratio);
            }
            return g;
        }

        double shareForStrategy(const std::vector<std::string>& names,
            const std::vector<int>& counts,
            int population,
            const std::string& strategy) {
            if (population <= 0) return 0.0;
            for (std::size_t i = 0; i < names.size() && i < counts.size(); ++i) {
                if (names[i] == strategy) {
                    return static_cast<double>(counts[i]) / static_cast<double>(population);
                }
            }
            return 0.0;
        }

        std::vector<double> computeProbabilities(const std::vector<int>& counts,
            const std::vector<double>& fitness) {
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
                total = 0.0;
                for (std::size_t i = 0; i < n; ++i) {
                    const double w = static_cast<double>(std::max(counts[i], 0));
                    weights[i] = w;
                    total += w;
                }
                if (total <= 0.0) {
                    const double u = 1.0 / static_cast<double>(n);
                    return std::vector<double>(n, u);
                }
            }

            std::vector<double> p(n, 0.0);
            for (std::size_t i = 0; i < n; ++i) p[i] = weights[i] / total;
            return p;
        }

        std::vector<int> allocateNextGeneration(const std::vector<double>& probabilities,
            int population) {
            const std::size_t n = probabilities.size();
            std::vector<int> counts(n, 0);
            if (n == 0 || population <= 0) return counts;

            std::vector<double> expected(n, 0.0);
            int assigned = 0;
            for (std::size_t i = 0; i < n; ++i) {
                const double e = probabilities[i] * static_cast<double>(population);
                expected[i] = e;
                const int base = static_cast<int>(std::floor(e));
                counts[i] = base;
                assigned += base;
            }

            int remainder = population - assigned;
            if (remainder == 0) return counts;

            struct Fractional { std::size_t index; double frac; };
            std::vector<Fractional> fracs;
            fracs.reserve(n);
            for (std::size_t i = 0; i < n; ++i) {
                const double frac = expected[i] - static_cast<double>(counts[i]);
                fracs.push_back(Fractional{ i, frac });
            }

            std::sort(fracs.begin(), fracs.end(), [](const Fractional& a, const Fractional& b) {
                if (std::abs(a.frac - b.frac) <= kEpsilon) return a.index < b.index;
                return a.frac > b.frac;
                });

            if (remainder > 0) {
                const int give = std::min<int>(remainder, static_cast<int>(fracs.size()));
                for (int k = 0; k < give; ++k) counts[fracs[k].index] += 1;
            }
            else {
                const int take = std::min<int>(-remainder, static_cast<int>(fracs.size()));
                for (int k = 0; k < take; ++k) {
                    const std::size_t idx = fracs[fracs.size() - 1 - k].index;
                    if (counts[idx] > 0) counts[idx] -= 1;
                }
            }
            return counts;
        }

        void mutateCounts(std::vector<int>& counts,
            double mutationRate,
            const std::vector<double>& probabilities) {
            const std::size_t n = counts.size();
            if (n <= 1 || mutationRate <= 0.0) return;

            std::vector<int> migrants(n, 0);
            int totalMigrants = 0;
            for (std::size_t i = 0; i < n; ++i) {
                const int c = counts[i];
                if (c <= 0) { migrants[i] = 0; continue; }
                int m = static_cast<int>(std::llround(static_cast<double>(c) * mutationRate));
                m = std::clamp(m, 0, c);
                migrants[i] = m;
                counts[i] -= m;
                totalMigrants += m;
            }
            if (totalMigrants == 0) return;

            std::vector<double> incoming(n, 0.0);
            for (std::size_t src = 0; src < n; ++src) {
                const int m = migrants[src];
                if (m == 0) continue;

                double denom = 0.0;
                for (std::size_t t = 0; t < n; ++t) if (t != src && t < probabilities.size()) denom += probabilities[t];

                if (denom <= 0.0) {
                    const double share = 1.0 / static_cast<double>(n - 1);
                    for (std::size_t t = 0; t < n; ++t) if (t != src) incoming[t] += static_cast<double>(m) * share;
                    continue;
                }

                for (std::size_t t = 0; t < n; ++t) {
                    if (t == src) continue;
                    const double p = (t < probabilities.size() ? probabilities[t] : 0.0) / denom;
                    incoming[t] += static_cast<double>(m) * p;
                }
            }

            std::vector<std::pair<std::size_t, double>> remainders;
            remainders.reserve(n);
            int assigned = 0;
            for (std::size_t i = 0; i < n; ++i) {
                const int base = static_cast<int>(std::floor(incoming[i]));
                counts[i] += base;
                assigned += base;
                remainders.emplace_back(i, incoming[i] - static_cast<double>(base));
            }

            int remain = totalMigrants - assigned;
            if (remain > 0) {
                std::sort(remainders.begin(), remainders.end(), [](const auto& a, const auto& b) {
                    if (std::abs(a.second - b.second) <= kEpsilon) return a.first < b.first;
                    return a.second > b.second;
                    });
                const int give = std::min<int>(remain, static_cast<int>(remainders.size()));
                for (int k = 0; k < give; ++k) counts[remainders[k].first] += 1;
            }
        }

        void writeEvolutionSharesCsv(const Config& config, const std::vector<GenerationShare>& history) {
            if (history.empty()) return;

            namespace fs = std::filesystem;
            fs::path out = config.outputFile.empty() ? fs::path("evolution_shares.csv")
                : fs::path(config.outputFile).parent_path() / "evolution_shares.csv";
            if (!out.parent_path().empty() && !fs::exists(out.parent_path())) {
                fs::create_directories(out.parent_path());
            }

            std::ofstream os(out, std::ios::trunc);
            if (!os) throw std::runtime_error("Unable to write evolution share log: " + out.string());

            os << "generation,strategy,count,share\n";
            for (const auto& g : history) {
                std::map<std::string, int> countLookup;
                for (const auto& kv : g.counts) countLookup[kv.first] = kv.second;
                for (const auto& kv : g.shares) {
                    const std::string& name = kv.first;
                    const double share = kv.second;
                    const int cnt = countLookup[name];
                    os << g.generation << ',' << name << ',' << cnt << ','
                        << std::fixed << std::setprecision(6) << share << '\n';
                }
            }
        }

    } 

    EvolutionOutcome EvolutionManager::run(const Config& config) const {
        EvolutionOutcome out;

        if (!config.evolve && config.generations <= 0) return out;

        registerBuiltinStrategies();

        const std::size_t strategyCount = config.strategyNames.size();
        if (strategyCount == 0) return out;

        const int population = std::max(config.populationSize, 0);

        std::vector<int> counts = initialCounts(config.strategyNames, population);

        TournamentManager tm;
        std::vector<Result> lastFitness;
        const Config evalCfg = makeEvaluationConfig(config);

        out.history.push_back(makeGenerationShare(0, config.strategyNames, counts, population));

        for (int gen = 0; gen < config.generations; ++gen) {
            lastFitness = tm.run(evalCfg);

            const std::vector<double> fitness = collectFitness(config.strategyNames, lastFitness, config.complexityPenalty);

            const std::vector<double> probs = computeProbabilities(counts, fitness);

            std::vector<int> nextCounts = allocateNextGeneration(probs, population);

            mutateCounts(nextCounts, config.mutationRate, probs);

            counts = std::move(nextCounts);
            out.history.push_back(makeGenerationShare(gen + 1, config.strategyNames, counts, population));
        }

        if (lastFitness.empty()) lastFitness = tm.run(evalCfg);

        for (auto& r : lastFitness) {
            r.extra = shareForStrategy(config.strategyNames, counts, population, r.strategy);
        }

        out.results = std::move(lastFitness);
        return out;
    }

} 