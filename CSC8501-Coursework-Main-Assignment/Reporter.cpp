#include "Reporter.h"

#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <sstream>
#include <string_view>
#include <tuple>

namespace ipd {
    namespace {
        constexpr int kStrategyWidth = 18;
        constexpr int kNumericWidth = 12;
        constexpr int kSamplesWidth = 10;

        struct Column {
            std::string header;
            int width;
            bool leftAlign;
            std::function<std::string(const Result&)> accessor;
        };

        using ColumnArray = std::array<Column, 8>;
        constexpr std::size_t kColumnCount = std::tuple_size<ColumnArray>::value;

        ColumnArray columns() {
            return ColumnArray{
                Column{"Strategy", kStrategyWidth, true, [](const Result& result) { return result.strategy; }},
                Column{"Mean", kNumericWidth, false, [](const Result& result) {
                    std::ostringstream stream;
                    stream << std::fixed << std::setprecision(3) << result.mean;
                    return stream.str();
                }},
                Column{"Variance", kNumericWidth, false, [](const Result& result) {
                    std::ostringstream stream;
                    stream << std::fixed << std::setprecision(3) << result.variance;
                    return stream.str();
                }},
                Column{"CI Low", kNumericWidth, false, [](const Result& result) {
                    std::ostringstream stream;
                    stream << std::fixed << std::setprecision(3) << result.ciLow;
                    return stream.str();
                }},
                Column{"CI High", kNumericWidth, false, [](const Result& result) {
                    std::ostringstream stream;
                    stream << std::fixed << std::setprecision(3) << result.ciHigh;
                    return stream.str();
                }},
                Column{"Complexity", kNumericWidth, false, [](const Result& result) {
                    std::ostringstream stream;
                    stream << std::fixed << std::setprecision(3) << result.complexity;
                    return stream.str();
                }},
                Column{"Samples", kSamplesWidth, false, [](const Result& result) {
                    if (result.samples == 0) {
                        return std::string();
                    }
                    return std::to_string(result.samples);
                }},
                Column{"Extra", kNumericWidth, false, [](const Result& result) {
                    if (result.extra == 0.0) {
                        return std::string();
                    }
                    std::ostringstream stream;
                    stream << std::fixed << std::setprecision(3) << result.extra;
                    return stream.str();
                }}
            };
        }

        std::size_t tableWidth(const ColumnArray& columns) {
            return std::accumulate(columns.begin(), columns.end(), std::size_t{1}, [](std::size_t total, const Column& column) {
                return total + static_cast<std::size_t>(column.width + 3);
            });
        }

        void printSeparator(std::ostream& stream, std::size_t width) {
            stream << std::string(width, '-') << '\n';
        }

        std::string formatCell(const Column& column, const std::string& value) {
            std::ostringstream cell;
            if (column.leftAlign) {
                cell << std::left;
            }
            else {
                cell << std::right;
            }
            cell << std::setw(column.width) << value;
            return cell.str();
        }

        void printRow(std::ostream& stream, const ColumnArray& columns, const std::array<std::string, kColumnCount>& values) {
            stream << '|';
            for (std::size_t index = 0; index < columns.size(); ++index) {
                stream << ' ' << formatCell(columns[index], values[index]) << ' ' << '|';
            }
            stream << '\n';
        }

        void printHeader(std::ostream& stream, const ColumnArray& columns) {
            std::array<std::string, kColumnCount> headers{};
            std::transform(columns.begin(), columns.end(), headers.begin(), [](const Column& column) {
                return column.header;
            });
            printRow(stream, columns, headers);
        }

        void printRows(std::ostream& stream, const ColumnArray& columns, const std::vector<Result>& results) {
            std::for_each(results.begin(), results.end(), [&](const Result& result) {
                std::array<std::string, kColumnCount> values{};
                std::transform(columns.begin(), columns.end(), values.begin(), [&](const Column& column) {
                    return column.accessor(result);
                });
                printRow(stream, columns, values);
            });
        }

        void printResultsTable(std::ostream& stream, std::string_view title, const std::vector<Result>& results) {
            stream << title << '\n';
            const auto columnDefinitions = columns();
            const auto width = tableWidth(columnDefinitions);
            printSeparator(stream, width);
            printHeader(stream, columnDefinitions);
            printSeparator(stream, width);
            printRows(stream, columnDefinitions, results);
            printSeparator(stream, width);
        }

        class ConsoleReporter : public Reporter {
        public:
            void reportTournament(const Config& config, const std::vector<Result>& results) override {
                (void)config;
                printResultsTable(std::cout, "=== Tournament Results ===", results);
                std::cout.flush();
            }

            void reportEvolution(const Config& config, const std::vector<Result>& results) override {
                (void)config;
                printResultsTable(std::cout, "=== Evolution Results ===", results);
                std::cout.flush();
            }
        };

        class CsvReporter : public Reporter {
        public:
            explicit CsvReporter(std::string path)
                : m_path(std::move(path)) {}

            void reportTournament(const Config& config, const std::vector<Result>& results) override {
                writeFile("tournament", config, results);
            }

            void reportEvolution(const Config& config, const std::vector<Result>& results) override {
                writeFile("evolution", config, results);
            }

        private:
            void writeFile(const std::string& prefix, const Config&, const std::vector<Result>& results) {
                namespace fs = std::filesystem;
                fs::path directory = fs::path("data");
                if (!m_path.empty()) {
                    directory = fs::path(m_path).parent_path();
                    if (directory.empty()) {
                        directory = fs::path("data");
                    }
                }
                if (!fs::exists(directory)) {
                    fs::create_directories(directory);
                }

                fs::path filePath = m_path.empty() ? directory / (prefix + ".csv") : fs::path(m_path);
                std::ofstream file(filePath, std::ios::trunc);
                file << "strategy,mean,variance,ci_low,ci_high,complexity,samples,extra\n";
                std::for_each(results.begin(), results.end(), [&](const Result& result) {
                    file << result.toCsv() << '\n';
                });
            }

            std::string m_path;
        };
    }

    ReporterPtr makeConsoleReporter() {
        return std::make_unique<ConsoleReporter>();
    }

    ReporterPtr makeCsvReporter(const std::string& path) {
        return std::make_unique<CsvReporter>(path);
    }
}