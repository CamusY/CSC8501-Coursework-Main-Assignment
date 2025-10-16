#include "Reporter.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <cmath>
#include <functional>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace ipd {
    namespace {
        struct Column {
            std::string header;
            int width;
            bool leftAlign;
            std::function<std::string(const Result&)> accessor;
        };

        using ColumnList = std::vector<Column>;

        std::string formatPayoffs(const Payoff& payoffs) {
            std::ostringstream stream;
            stream << '(' << payoffs.T << ',' << payoffs.R << ',' << payoffs.P << ',' << payoffs.S << ')';
            return stream.str();
        }

        std::string formatShareList(const std::vector<std::pair<std::string, double>>& shares) {
            std::ostringstream stream;
            bool first = true;
            for (const auto& [strategy, value] : shares) {
                if (!first) {
                    stream << ", ";
                }
                first = false;
                stream << strategy << '=' << std::fixed << std::setprecision(3) << value;
            }
            return stream.str();
        }

        std::string formatFirstDefectionCell(const Result& r) {
            if (!r.firstDefection) {
                return "NA";
            }
            std::ostringstream s;
            s << std::fixed << std::setprecision(3) << *r.firstDefection;
            return s.str();
        }

        std::string formatCostValue(double cost) {
            std::ostringstream s;
            s << static_cast<int>(std::llround(cost));
            return s.str();
        }

        std::vector<std::pair<std::string, double>> collectCostEntries(const std::vector<Result>& results) {
            std::vector<std::pair<std::string, double>> entries;
            entries.reserve(results.size());
            for (const auto& r : results) {
                entries.emplace_back(r.strategy, r.cost);
            }
            std::sort(entries.begin(), entries.end(), [](const auto& lhs, const auto& rhs) {
                return lhs.first < rhs.first;
                });
            return entries;
        }

        std::string formatCostMapping(const std::vector<Result>& results) {
            const auto entries = collectCostEntries(results);
            std::ostringstream stream;
            bool first = true;
            for (const auto& [name, cost] : entries) {
                if (!first) {
                    stream << ", ";
                }
                first = false;
                stream << name << '=' << static_cast<int>(std::llround(cost));
            }
            return stream.str();
        }

        std::string escapeJson(std::string_view text);

        std::string jsonCostObject(const std::vector<Result>& results) {
            const auto entries = collectCostEntries(results);
            std::ostringstream stream;
            stream << '{';
            for (std::size_t index = 0; index < entries.size(); ++index) {
                const auto& [name, cost] = entries[index];
                if (index != 0) {
                    stream << ',';
                }
                stream << "\"" << escapeJson(name) << "\": " << static_cast<int>(std::llround(cost));
            }
            stream << '}';
            return stream.str();
        }

        ColumnList resultColumns(const Config& config) {
            ColumnList columns{
                { "Strategy", 18, true, [](const Result& r) { return r.strategy; } },
                { config.scbEnabled ? "RawMean" : "Mean", 12, false, [](const Result& r) {
                    std::ostringstream s;
                    s << std::fixed << std::setprecision(3) << r.mean;
                    return s.str();
                } }
            };
            if (config.scbEnabled) {
                columns.push_back({ "NetMean", 12, false, [](const Result& r) {
                    std::ostringstream s;
                    s << std::fixed << std::setprecision(3) << r.netMean;
                    return s.str();
                } });
                columns.push_back({ "Cost", 8, false, [](const Result& r) {
                    return formatCostValue(r.cost);
                } });
            }
            const ColumnList tail{
                { "StdDev", 12, false, [](const Result& r) {
                    std::ostringstream s;
                    s << std::fixed << std::setprecision(3) << r.stdev;
                    return s.str();
                } },
                { "CI Low", 12, false, [](const Result& r) {
                    std::ostringstream s;
                    s << std::fixed << std::setprecision(3) << r.ciLow;
                    return s.str();
                } },
                { "CI High", 12, false, [](const Result& r) {
                    std::ostringstream s;
                    s << std::fixed << std::setprecision(3) << r.ciHigh;
                    return s.str();
                } },
                { "CoopRate", 12, false, [](const Result& r) {
                    std::ostringstream s;
                    s << std::fixed << std::setprecision(3) << r.coopRate;
                    return s.str();
                } },
                { "FirstDef", 12, false, [](const Result& r) {
                    return formatFirstDefectionCell(r);
                } },
                { "EchoLen", 12, false, [](const Result& r) {
                    std::ostringstream s;
                    s << std::fixed << std::setprecision(3) << r.echoLength;
                    return s.str();
                } },
                { "Complexity", 12, false, [](const Result& r) {
                    std::ostringstream s;
                    s << std::fixed << std::setprecision(3) << r.complexity;
                    return s.str();
                } },
                { "Samples", 10, false, [](const Result& r) {
                    return r.samples == 0 ? std::string{} : std::to_string(r.samples);
                } },
                { "Share", 10, false, [](const Result& r) {
                    if (r.extra <= 0.0) {
                        return std::string();
                    }
                    std::ostringstream s;
                    s << std::fixed << std::setprecision(3) << r.extra;
                    return s.str();
                } }
            };
            columns.insert(columns.end(), tail.begin(), tail.end());
            return columns;
        }
        int tableWidth(const ColumnList& columns) {
            return std::accumulate(columns.begin(), columns.end(), 1, [](int total, const Column& column) {
                return total + column.width + 3;
                });
        }

        std::string formatCell(const Column& column, const std::string& value) {
            std::ostringstream stream;
            stream << (column.leftAlign ? std::left : std::right);
            stream << std::setw(column.width) << value;
            return stream.str();
        }

        void appendSeparator(std::ostringstream& buffer, int width) {
            buffer << std::string(static_cast<std::size_t>(width), '-') << '\n';
        }

        void appendRow(std::ostringstream& buffer, const ColumnList& columns, const std::vector<std::string>& values) {
            buffer << '|';
            for (std::size_t index = 0; index < columns.size(); ++index) {
                buffer << ' ' << formatCell(columns[index], values[index]) << ' ' << '|';
            }
            buffer << '\n';
        }

        void appendHeader(std::ostringstream& buffer, const ColumnList& columns) {
            std::vector<std::string> headers;
            headers.reserve(columns.size());
            std::transform(columns.begin(), columns.end(), std::back_inserter(headers), [](const Column& column) {
                return column.header;
                });
            appendRow(buffer, columns, headers);
        }

        void appendResults(std::ostringstream& buffer, const ColumnList& columns, const std::vector<Result>& results) {
            std::vector<std::string> values(columns.size());
            for (const auto& result : results) {
                std::transform(columns.begin(), columns.end(), values.begin(), [&](const Column& column) {
                    return column.accessor(result);
                    });
                appendRow(buffer, columns, values);
            }
        }

        std::string buildTextReport(const Config& config, const std::vector<Result>& results, const std::vector<GenerationShare>& history) {
            std::ostringstream buffer;
            buffer << "Seed=" << (config.useSeed ? std::to_string(config.seed) : std::string("random"));
            buffer << ", Epsilon=" << std::fixed << std::setprecision(3) << config.epsilon;
            buffer << ", Payoffs=" << formatPayoffs(config.payoffs) << '\n';
            buffer << "Rounds=" << config.rounds << ", Repeats=" << config.repeats;
            if (config.evolve) {
                buffer << ", Generations=" << config.generations;
            }
            buffer << '\n';
            buffer << "SCB=" << (config.scbEnabled ? "enabled" : "disabled");
            if (config.scbEnabled) {
                const std::string mapping = formatCostMapping(results);
                if (!mapping.empty()) {
                    buffer << ", Costs=" << mapping;
                }
            }
            buffer << '\n';

            const auto columns = resultColumns(config);
            const int width = tableWidth(columns);
            appendSeparator(buffer, width);
            appendHeader(buffer, columns);
            appendSeparator(buffer, width);
            appendResults(buffer, columns, results);
            appendSeparator(buffer, width);

            if (!history.empty()) {
                const auto& first = history.front();
                const auto& last = history.back();
                buffer << "Generation " << first.generation << ": " << formatShareList(first.shares) << '\n';
                if (last.generation != first.generation) {
                    buffer << "Generation " << last.generation << ": " << formatShareList(last.shares) << '\n';
                }
            }
            return buffer.str();
        }

        std::ostream& prepareStream(const Config& config, std::ofstream& file) {
            if (config.outputFile.empty()) {
                return std::cout;
            }
            namespace fs = std::filesystem;
            fs::path path(config.outputFile);
            if (!path.parent_path().empty() && !fs::exists(path.parent_path())) {
                fs::create_directories(path.parent_path());
            }
            file.open(path, std::ios::trunc);
            if (!file) {
                throw std::runtime_error("Unable to open output file: " + path.string());
            }
            return file;
        }

        void writeCsvReport(const Config& config, const std::vector<Result>& results) {
            std::ofstream file;
            std::ostream& stream = prepareStream(config, file);
            stream << "strategy,mean";
            if (config.scbEnabled) {
                stream << ",net_mean,cost";
            }
            stream << ",stdev,ci95_low,ci95_high,coop_rate,first_defection,echo_length,repeats,seed,epsilon,payoffs,complexity,samples,share\n";
            const std::string payoffs = std::to_string(config.payoffs.T) + ',' + std::to_string(config.payoffs.R) + ',' +
                std::to_string(config.payoffs.P) + ',' + std::to_string(config.payoffs.S);
            for (const auto& result : results) {
                stream << '"' << result.strategy << '"' << ','
                    << std::fixed << std::setprecision(6) << result.mean;
                if (config.scbEnabled) {
                    stream << ',' << result.netMean << ',' << result.cost;
                }
                stream << ',' << result.stdev << ','
                    << result.ciLow << ','
                    << result.ciHigh << ','
                    << result.coopRate << ',';
                if (result.firstDefection) {
                    stream << *result.firstDefection;
                }
                else {
                    stream << "NA";
                }
                stream << ',' << result.echoLength << ','
                    << config.repeats << ','
                    << (config.useSeed ? std::to_string(config.seed) : std::string()) << ','
                    << config.epsilon << ','
                    << '"' << payoffs << '"' << ','
                    << result.complexity << ','
                    << result.samples << ','
                    << result.extra << '\n';
            }
        }

        std::string escapeJson(std::string_view text) {
            std::string escaped;
            escaped.reserve(text.size());
            for (char ch : text) {
                switch (ch) {
                case '"': escaped += "\\\""; break;
                case '\\': escaped += "\\\\"; break;
                case '\n': escaped += "\\n"; break;
                case '\r': escaped += "\\r"; break;
                case '\t': escaped += "\\t"; break;
                default: escaped.push_back(ch); break;
                }
            }
            return escaped;
        }

        std::string strategyArray(const std::vector<std::string>& strategies) {
            std::ostringstream buffer;
            buffer << '[';
            for (std::size_t index = 0; index < strategies.size(); ++index) {
                if (index != 0) {
                    buffer << ',';
                }
                buffer << '"' << escapeJson(strategies[index]) << '"';
            }
            buffer << ']';
            return buffer.str();
        }

        void writeJsonReport(const Config& config, const std::vector<Result>& results, const std::vector<GenerationShare>& history) {
            std::ofstream file;
            std::ostream& stream = prepareStream(config, file);
            stream << "{\n";
            stream << "  \"meta\": {\n";
            stream << "    \"rounds\": " << config.rounds << ",\n";
            stream << "    \"repeats\": " << config.repeats << ",\n";
            stream << "    \"epsilon\": " << config.epsilon << ",\n";
            stream << "    \"payoffs\": [" << config.payoffs.T << ',' << config.payoffs.R << ',' << config.payoffs.P << ',' << config.payoffs.S << "],\n";
            stream << "    \"seed\": " << (config.useSeed ? std::to_string(config.seed) : "null") << ",\n";
            stream << "    \"strategies\": " << strategyArray(config.strategyNames) << ",\n";
            stream << "    \"scb_enabled\": " << (config.scbEnabled ? "true" : "false");
            if (config.scbEnabled) {
                stream << ",\n    \"scb_costs\": " << jsonCostObject(results) << '\n';
            }
            else {
                stream << '\n';
            }
            stream << "  },\n";
            stream << "  \"results\": [\n";
            for (std::size_t index = 0; index < results.size(); ++index) {
                const auto& result = results[index];
                stream << "    {\n";
                stream << "      \"strategy\": \"" << escapeJson(result.strategy) << "\",\n";
                stream << "      \"mean\": " << result.mean << ",\n";
                if (config.scbEnabled) {
                    stream << "      \"net_mean\": " << result.netMean << ",\n";
                    stream << "      \"cost\": " << result.cost << ",\n";
                }
                stream << "      \"stdev\": " << result.stdev << ",\n";
                stream << "      \"ci95_low\": " << result.ciLow << ",\n";
                stream << "      \"ci95_high\": " << result.ciHigh << ",\n";
                stream << "      \"coop_rate\": " << result.coopRate << ",\n";
                stream << "      \"first_defection\": ";
                if (result.firstDefection) {
                    stream << *result.firstDefection;
                }
                else {
                    stream << "null";
                }
                stream << ",\n";
                stream << "      \"echo_length\": " << result.echoLength << ",\n";
                stream << "      \"complexity\": " << result.complexity << ",\n";
                stream << "      \"samples\": " << result.samples << ",\n";
                stream << "      \"share\": " << result.extra << ",\n";
                stream << "      \"repeats\": " << config.repeats << '\n';
                stream << "    }" << (index + 1 == results.size() ? "\n" : ",\n");
            }
            stream << "  ]";
            if (!history.empty()) {
                stream << ",\n  \"evolution\": [\n";
                for (std::size_t index = 0; index < history.size(); ++index) {
                    const auto& entry = history[index];
                    stream << "    {\n";
                    stream << "      \"generation\": " << entry.generation << ",\n";
                    stream << "      \"shares\": [";
                    for (std::size_t shareIndex = 0; shareIndex < entry.shares.size(); ++shareIndex) {
                        const auto& share = entry.shares[shareIndex];
                        if (shareIndex != 0) {
                            stream << ',';
                        }
                        stream << "{\"strategy\":\"" << escapeJson(share.first) << "\",\"share\": " << share.second << '}';
                    }
                    stream << "]\n    }" << (index + 1 == history.size() ? "\n" : ",\n");
                }
                stream << "  ]\n";
            }
            else {
                stream << '\n';
            }
            stream << "}\n";
        }
    }

    void reportResults(const Config& config, const std::vector<Result>& results, const std::vector<GenerationShare>& history) {
        if (config.outputFormat == "csv") {
            writeCsvReport(config, results);
            return;
        }
        if (config.outputFormat == "json") {
            writeJsonReport(config, results, history);
            return;
        }
        const std::string report = buildTextReport(config, results, history);
        std::cout << report;
        std::cout.flush();
        if (!config.outputFile.empty()) {
            std::ofstream file;
            std::ostream& stream = prepareStream(config, file);
            stream << report;
        }
    }
}