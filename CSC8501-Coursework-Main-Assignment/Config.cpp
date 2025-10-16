#include "Config.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <map>
#include <optional>
#include <sstream>
#include <string_view>
#include <tuple>
#include <vector>
#include <stdexcept>
#include <unordered_map>

namespace ipd {
    namespace {
        using OptionalString = std::optional<std::string>;

        struct ConfigOverrides {
            std::optional<int> rounds;
            std::optional<int> repeats;
            std::optional<double> epsilon;
            std::optional<unsigned int> seed;
            std::optional<Payoff> payoffs;
            std::optional<std::vector<std::string>> strategies;
            std::optional<int> generations;
            std::optional<int> population;
            std::optional<double> mutation;
            std::optional<double> complexityPenalty;
            std::optional<std::string> format;
            std::optional<std::string> output;
            std::optional<bool> verbose;
            std::optional<bool> evolve;
            OptionalString saveFile;
            OptionalString loadFile;
            std::optional<bool> scbEnabled;
            std::optional<std::unordered_map<std::string, int>> scbCosts;
        };

        void exitWithError(const std::string& message) {
            std::cerr << message << '\n';
            std::exit(1);
        }

        void printHelpAndExit() {
            std::cout <<
                "Usage: ipd [options]\n"
                "  --rounds N\n"
                "  --repeats N\n"
                "  --epsilon FLOAT             # noise probability per move (0..1)\n"
                "  --strategies LIST           # e.g. ALLC,ALLD,TFT,GRIM,PAVLOV,RND(0.3),CTFT,PROBER,Empath,Reflector\n"
                "  --payoffs T,R,P,S           # e.g. 5,3,1,0\n"
                "  --evolve 0/1\n"
                "  --generations N\n"
                "  --population N\n"
                "  --mutation FLOAT\n"
                "  --format {text|csv|json}   # output format only\n"
                "  --output FILE              # output destination only (defaults to stdout)\n"
                "  --seed N\n"
                "  --save FILE                 # save effective config to JSON (includes scb)\n"
                "  --load FILE                 # load config from JSON (command line overrides loaded values)\n"
                "  --scb [MAP]                # enable SCB; no MAP uses default complexity; MAP overrides provided entries.\n"
                "                             #   e.g. --scb ALLC=1,ALLD=1,TFT=2,GRIM=2,PAVLOV=2,CTFT=3,PROBER=3,Empath=3,Reflector=3\n"
                "  --verbose\n"
                "  --help\n";
            std::cout.flush();
            std::exit(0);
        }

        bool startsWith(std::string_view text, std::string_view prefix) {
            return text.substr(0, prefix.size()) == prefix;
        }

        std::string trimCopy(std::string_view view) {
            while (!view.empty() && std::isspace(static_cast<unsigned char>(view.front()))) {
                view.remove_prefix(1);
            }
            while (!view.empty() && std::isspace(static_cast<unsigned char>(view.back()))) {
                view.remove_suffix(1);
            }
            return std::string(view);
        }

        template <typename T>
        T parseNumber(std::string_view text, std::string_view optionName) {
            std::stringstream stream{ std::string(text) };
            T value{};
            stream >> value;
            if (!stream || !stream.eof()) {
                exitWithError("error: invalid value for '" + std::string(optionName) + "'.");
            }
            return value;
        }

        std::vector<std::string> parseStrategies(std::string_view value) {
            std::stringstream stream{ std::string(value) };
            std::vector<std::string> names;
            std::string token;
            while (std::getline(stream, token, ',')) {
                auto trimmed = trimCopy(token);
                if (!trimmed.empty()) {
                    names.push_back(std::move(trimmed));
                }
            }
            if (names.empty()) {
                exitWithError("error: '--strategies' requires at least one strategy.");
            }
            return names;
        }

        Payoff parsePayoffs(std::string_view value) {
            std::array<double, 4> numbers{};
            std::stringstream stream{ std::string(value) };
            std::string token;
            for (std::size_t index = 0; index < numbers.size(); ++index) {
                if (!std::getline(stream, token, ',')) {
                    exitWithError("error: '--payoffs' requires four comma-separated values.");
                }
                numbers[index] = parseNumber<double>(trimCopy(token), "--payoffs");
            }
            if (stream.rdbuf()->in_avail() != 0) {
                exitWithError("error: '--payoffs' requires exactly four values.");
            }
            return Payoff(numbers[0], numbers[1], numbers[2], numbers[3]);
        }

        std::unordered_map<std::string, int> parseScbMap(std::string_view value) {
            std::unordered_map<std::string, int> costs;
            std::stringstream stream{ std::string(value) };
            std::string token;
            while (std::getline(stream, token, ',')) {
                const std::string entry = trimCopy(token);
                if (entry.empty()) {
                    continue;
                }
                const auto equalPos = entry.find('=');
                if (equalPos == std::string::npos) {
                    exitWithError("error: '--scb' mapping entries must use Strategy=Cost.");
                }
                std::string key = trimCopy(entry.substr(0, equalPos));
                std::string valueText = trimCopy(entry.substr(equalPos + 1));
                if (key.empty() || valueText.empty()) {
                    exitWithError("error: '--scb' mapping entries require non-empty key and value.");
                }
                const int cost = parseNumber<int>(valueText, "--scb");
                costs[key] = cost;
            }
            return costs;
        }

        OptionalString matchOptionValue(std::string_view argument, std::string_view optionName, int& index, int argc, char** argv) {
            const std::string prefix = std::string(optionName) + "=";
            if (argument == optionName) {
                if (index + 1 >= argc) {
                    exitWithError("error: missing value for '" + std::string(optionName) + "'.");
                }
                ++index;
                return std::string(argv[index]);
            }
            if (startsWith(argument, prefix)) {
                return std::string(argument.substr(prefix.size()));
            }
            return std::nullopt;
        }

        void applyOverrides(Config& config, const ConfigOverrides& overrides) {
            if (overrides.rounds) {
                config.rounds = *overrides.rounds;
            }
            if (overrides.repeats) {
                config.repeats = *overrides.repeats;
            }
            if (overrides.epsilon) {
                config.epsilon = *overrides.epsilon;
            }
            if (overrides.seed) {
                config.seed = *overrides.seed;
                config.useSeed = true;
            }
            if (overrides.payoffs) {
                config.payoffs = *overrides.payoffs;
            }
            if (overrides.strategies) {
                config.strategyNames = *overrides.strategies;
            }
            if (overrides.generations) {
                config.generations = *overrides.generations;
            }
            if (overrides.population) {
                config.populationSize = *overrides.population;
            }
            if (overrides.mutation) {
                config.mutationRate = *overrides.mutation;
            }
            if (overrides.complexityPenalty) {
                config.complexityPenalty = *overrides.complexityPenalty;
            }
            if (overrides.format) {
                config.outputFormat = *overrides.format;
            }
            if (overrides.output) {
                config.outputFile = *overrides.output;
            }
            if (overrides.verbose) {
                config.verbose = *overrides.verbose;
            }
            if (overrides.evolve) {
                config.evolve = *overrides.evolve;
            }
            if (overrides.saveFile) {
                config.saveFile = *overrides.saveFile;
            }
            if (overrides.loadFile) {
                config.loadFile = *overrides.loadFile;
            }
            if (overrides.scbEnabled) {
                config.scbEnabled = *overrides.scbEnabled;
            }
            if (overrides.scbCosts) {
                config.scbCosts = *overrides.scbCosts;
            }
        }

        std::string escapeJson(std::string_view text) {
            std::string escaped;
            escaped.reserve(text.size());
            for (char ch : text) {
                switch (ch) {
                case '\\': escaped += "\\\\"; break;
                case '"': escaped += "\\\""; break;
                case '\n': escaped += "\\n"; break;
                case '\r': escaped += "\\r"; break;
                case '\t': escaped += "\\t"; break;
                default: escaped.push_back(ch); break;
                }
            }
            return escaped;
        }

        std::optional<std::string> extractRawValue(const std::string& json, std::string_view key) {
            const std::string quotedKey = '"' + std::string(key) + '"';
            const auto keyPos = json.find(quotedKey);
            if (keyPos == std::string::npos) {
                return std::nullopt;
            }
            const auto colonPos = json.find(':', keyPos + quotedKey.size());
            if (colonPos == std::string::npos) {
                return std::nullopt;
            }
            const auto valuePos = json.find_first_not_of(" \t\n\r", colonPos + 1);
            if (valuePos == std::string::npos) {
                return std::nullopt;
            }
            if (json[valuePos] == '"') {
                auto end = json.find('"', valuePos + 1);
                while (end != std::string::npos && json[end - 1] == '\\') {
                    end = json.find('"', end + 1);
                }
                if (end == std::string::npos) {
                    return std::nullopt;
                }
                return json.substr(valuePos + 1, end - valuePos - 1);
            }
            if (json[valuePos] == '[' || json[valuePos] == '{') {
                const char open = json[valuePos];
                const char close = (open == '[') ? ']' : '}';
                int depth = 1;
                std::size_t index = valuePos + 1;
                bool inString = false;
                bool escape = false;
                while (index < json.size() && depth > 0) {
                    const char current = json[index];
                    if (inString) {
                        if (current == '"' && !escape) {
                            inString = false;
                        }
                        escape = (!escape && current == '\\');
                    }
                    else {
                        if (current == '"') {
                            inString = true;
                        }
                        else if (current == open) {
                            ++depth;
                        }
                        else if (current == close) {
                            --depth;
                        }
                    }
                    ++index;
                }
                return json.substr(valuePos, index - valuePos);
            }
            const auto endPos = json.find_first_of(",}\n\r", valuePos);
            return json.substr(valuePos, endPos - valuePos);
        }

        std::optional<int> parseIntField(const std::string& json, std::string_view key) {
            const auto raw = extractRawValue(json, key);
            if (!raw) {
                return std::nullopt;
            }
            return parseNumber<int>(trimCopy(*raw), key);
        }

        std::optional<double> parseDoubleField(const std::string& json, std::string_view key) {
            const auto raw = extractRawValue(json, key);
            if (!raw) {
                return std::nullopt;
            }
            return parseNumber<double>(trimCopy(*raw), key);
        }

        std::optional<bool> parseBoolField(const std::string& json, std::string_view key) {
            const auto raw = extractRawValue(json, key);
            if (!raw) {
                return std::nullopt;
            }
            const std::string value = trimCopy(*raw);
            if (value == "true") {
                return true;
            }
            if (value == "false") {
                return false;
            }
            exitWithError("error: invalid boolean value for '" + std::string(key) + "'.");
            return std::nullopt;
        }

        std::optional<std::string> parseStringField(const std::string& json, std::string_view key) {
            const auto raw = extractRawValue(json, key);
            if (!raw) {
                return std::nullopt;
            }
            return *raw;
        }

        std::optional<std::vector<std::string>> parseStrategiesField(const std::string& json, std::string_view key) {
            const auto raw = extractRawValue(json, key);
            if (!raw) {
                return std::nullopt;
            }
            if (raw->empty() || raw->front() != '[' || raw->back() != ']') {
                exitWithError("error: invalid strategies array in JSON config.");
            }
            std::vector<std::string> strategies;
            std::string content = raw->substr(1, raw->size() - 2);
            std::stringstream stream{ content };
            std::string token;
            while (std::getline(stream, token, ',')) {
                token = trimCopy(token);
                if (token.size() >= 2 && token.front() == '"' && token.back() == '"') {
                    strategies.push_back(token.substr(1, token.size() - 2));
                }
            }
            return strategies;
        }
    }

    std::optional<std::unordered_map<std::string, int>> parseIntMapField(const std::string& json, std::string_view key) {
        const auto raw = extractRawValue(json, key);
        if (!raw) {
            return std::nullopt;
        }
        const std::string trimmed = trimCopy(*raw);
        if (trimmed.empty()) {
            return std::unordered_map<std::string, int>{};
        }
        if (trimmed.front() != '{' || trimmed.back() != '}') {
            exitWithError("error: invalid object for '" + std::string(key) + "'.");
        }
        std::unordered_map<std::string, int> costs;
        std::string content = trimmed.substr(1, trimmed.size() - 2);
        std::stringstream stream{ content };
        std::string token;
        while (std::getline(stream, token, ',')) {
            std::string entry = trimCopy(token);
            if (entry.empty()) {
                continue;
            }
            const auto colon = entry.find(':');
            if (colon == std::string::npos) {
                exitWithError("error: invalid entry in '" + std::string(key) + "'.");
            }
            std::string keyPart = trimCopy(entry.substr(0, colon));
            std::string valuePart = trimCopy(entry.substr(colon + 1));
            if (!keyPart.empty() && keyPart.front() == '"' && keyPart.back() == '"') {
                keyPart = keyPart.substr(1, keyPart.size() - 2);
            }
            if (keyPart.empty() || valuePart.empty()) {
                exitWithError("error: invalid entry in '" + std::string(key) + "'.");
            }
            const int cost = parseNumber<int>(valuePart, key);
            costs[keyPart] = cost;
        }
        return costs;
    }
 
    Config Config::fromCommandLine(int argc, char** argv) {
        Config config;
        ConfigOverrides overrides;

        for (int index = 1; index < argc; ++index) {
            std::string_view argument(argv[index]);

            if (argument == "--help") {
                printHelpAndExit();
            }
            if (argument == "--verbose") {
                overrides.verbose = true;
                continue;
            }
            if (argument == "--noise" || startsWith(argument, "--noise=")) {
                exitWithError("error: '--noise' has been removed. Use '--epsilon'.");
            }
            if (argument == "--payoff" || startsWith(argument, "--payoff=")) {
                exitWithError("error: '--payoff' has been removed. Use '--payoffs T,R,P,S'.");
            }

            if (startsWith(argument, "--scb=")) {
                overrides.scbEnabled = true;
                const std::string mapValue = trimCopy(argument.substr(6));
                if (mapValue.empty()) {
                    overrides.scbCosts = std::unordered_map<std::string, int>{};
                }
                else {
                    overrides.scbCosts = parseScbMap(mapValue);
                }
                continue;
            }
            if (argument == "--scb") {
                overrides.scbEnabled = true;
                std::unordered_map<std::string, int> costs;
                if (index + 1 < argc) {
                    std::string_view nextArgument(argv[index + 1]);
                    if (!nextArgument.empty() && nextArgument.front() != '-') {
                        ++index;
                        costs = parseScbMap(trimCopy(nextArgument));
                    }
                }
                overrides.scbCosts = std::move(costs);
                continue;
            }

            if (auto value = matchOptionValue(argument, "--rounds", index, argc, argv)) {
                overrides.rounds = parseNumber<int>(trimCopy(*value), "--rounds");
                continue;
            }
            if (auto value = matchOptionValue(argument, "--repeats", index, argc, argv)) {
                overrides.repeats = parseNumber<int>(trimCopy(*value), "--repeats");
                continue;
            }
            if (auto value = matchOptionValue(argument, "--epsilon", index, argc, argv)) {
                overrides.epsilon = parseNumber<double>(trimCopy(*value), "--epsilon");
                continue;
            }
            if (auto value = matchOptionValue(argument, "--seed", index, argc, argv)) {
                overrides.seed = static_cast<unsigned int>(parseNumber<unsigned long>(trimCopy(*value), "--seed"));
                continue;
            }
            if (auto value = matchOptionValue(argument, "--strategies", index, argc, argv)) {
                overrides.strategies = parseStrategies(*value);
                continue;
            }
            if (auto value = matchOptionValue(argument, "--payoffs", index, argc, argv)) {
                overrides.payoffs = parsePayoffs(*value);
                continue;
            }
            if (auto value = matchOptionValue(argument, "--generations", index, argc, argv)) {
                overrides.generations = parseNumber<int>(trimCopy(*value), "--generations");
                continue;
            }
            if (auto value = matchOptionValue(argument, "--population", index, argc, argv)) {
                overrides.population = parseNumber<int>(trimCopy(*value), "--population");
                continue;
            }
            if (auto value = matchOptionValue(argument, "--mutation", index, argc, argv)) {
                overrides.mutation = parseNumber<double>(trimCopy(*value), "--mutation");
                continue;
            }
            if (auto value = matchOptionValue(argument, "--penalty", index, argc, argv)) {
                overrides.complexityPenalty = parseNumber<double>(trimCopy(*value), "--penalty");
                continue;
            }
            if (auto value = matchOptionValue(argument, "--format", index, argc, argv)) {
                std::string format = trimCopy(*value);
                std::transform(format.begin(), format.end(), format.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
                if (format != "text" && format != "csv" && format != "json") {
                    exitWithError("error: '--format' must be one of text, csv, or json.");
                }
                overrides.format = format;
                continue;
            }
            if (auto value = matchOptionValue(argument, "--output", index, argc, argv)) {
                overrides.output = trimCopy(*value);
                continue;
            }
            if (auto value = matchOptionValue(argument, "--evolve", index, argc, argv)) {
                const int evolveFlag = parseNumber<int>(trimCopy(*value), "--evolve");
                if (evolveFlag != 0 && evolveFlag != 1) {
                    exitWithError("error: '--evolve' accepts only 0 or 1.");
                }
                overrides.evolve = evolveFlag == 1;
                continue;
            }
            if (auto value = matchOptionValue(argument, "--save", index, argc, argv)) {
                overrides.saveFile = trimCopy(*value);
                continue;
            }
            if (auto value = matchOptionValue(argument, "--load", index, argc, argv)) {
                overrides.loadFile = trimCopy(*value);
                continue;
            }

            throw std::runtime_error("Unknown command line argument: " + std::string(argument));
        }

        if (overrides.loadFile && !overrides.loadFile->empty()) {
            Config::loadFromJson(*overrides.loadFile, config);
            config.loadFile = *overrides.loadFile;
        }

        applyOverrides(config, overrides);
        config.ensureDefaults();
        return config;
    }

    void Config::ensureDefaults() {
        rounds = std::max(1, rounds);
        repeats = std::max(1, repeats);
        epsilon = std::clamp(epsilon, 0.0, 1.0);
        populationSize = std::max(0, populationSize);
        mutationRate = std::clamp(mutationRate, 0.0, 1.0);
        complexityPenalty = std::max(0.0, complexityPenalty);
        std::transform(outputFormat.begin(), outputFormat.end(), outputFormat.begin(), [](unsigned char ch) {
            return static_cast<char>(std::tolower(ch));
            });
        if (outputFormat != "text" && outputFormat != "csv" && outputFormat != "json") {
            outputFormat = "text";
        }
        if (strategyNames.empty()) {
            strategyNames = { "ALLC", "ALLD", "TFT", "GRIM", "PAVLOV", "RND", "CTFT", "PROBER", "Empath", "Reflector" };
        }
        if (populationSize == 0 && generations > 0) {
            populationSize = static_cast<int>(strategyNames.size());
        }
        evolve = evolve || generations > 0;
    }

    void Config::saveToJson(const std::string& path) const {
        if (path.empty()) {
            return;
        }
        std::ofstream stream(path, std::ios::trunc);
        if (!stream) {
            throw std::runtime_error("Unable to open file for writing: " + path);
        }

        const auto joinedStrategies = [&]() {
            std::ostringstream buffer;
            for (std::size_t index = 0; index < strategyNames.size(); ++index) {
                if (index != 0) {
                    buffer << ", ";
                }
                buffer << '"' << escapeJson(strategyNames[index]) << '"';
            }
            return buffer.str();
            }();

        const auto formattedScb = [&]() {
            std::ostringstream buffer;
            buffer << '{';
            bool first = true;
            for (const auto& [name, cost] : scbCosts) {
                if (!first) {
                    buffer << ", ";
                }
                first = false;
                buffer << '"' << escapeJson(name) << "\": " << cost;
            }
            buffer << '}';
            return buffer.str();
            }();

        stream << "{\n";
        stream << "  \"rounds\": " << rounds << ",\n";
        stream << "  \"repeats\": " << repeats << ",\n";
        stream << "  \"epsilon\": " << std::fixed << std::setprecision(6) << epsilon << ",\n";
        stream << "  \"payoffs\": \"" << payoffs.T << ',' << payoffs.R << ',' << payoffs.P << ',' << payoffs.S << "\",\n";
        stream << "  \"strategies\": [" << joinedStrategies << "],\n";
        stream << "  \"generations\": " << generations << ",\n";
        stream << "  \"population\": " << populationSize << ",\n";
        stream << "  \"mutation\": " << std::fixed << std::setprecision(6) << mutationRate << ",\n";
        stream << "  \"complexity_penalty\": " << std::fixed << std::setprecision(6) << complexityPenalty << ",\n";
        stream << "  \"scb_enabled\": " << (scbEnabled ? "true" : "false") << ",\n";
        stream << "  \"scb_costs\": " << formattedScb << ",\n";
        stream << "  \"format\": \"" << escapeJson(outputFormat) << "\",\n";
        stream << "  \"output\": \"" << escapeJson(outputFile) << "\",\n";
        stream << "  \"seed\": " << seed << ",\n";
        stream << "  \"use_seed\": " << (useSeed ? "true" : "false") << ",\n";
        stream << "  \"evolve\": " << (evolve ? "true" : "false") << ",\n";
        stream << "  \"verbose\": " << (verbose ? "true" : "false") << '\n';
        stream << "}\n";
    }

    void Config::loadFromJson(const std::string& path, Config& config) {
        std::ifstream stream(path);
        if (!stream) {
            throw std::runtime_error("Unable to open configuration file: " + path);
        }
        std::string json((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());

        if (auto value = parseIntField(json, "rounds")) {
            config.rounds = *value;
        }
        if (auto value = parseIntField(json, "repeats")) {
            config.repeats = *value;
        }
        if (auto value = parseDoubleField(json, "epsilon")) {
            config.epsilon = *value;
        }
        if (auto value = parseStringField(json, "payoffs")) {
            config.payoffs = parsePayoffs(*value);
        }
        if (auto value = parseStrategiesField(json, "strategies")) {
            config.strategyNames = *value;
        }
        if (auto value = parseIntField(json, "generations")) {
            config.generations = *value;
        }
        if (auto value = parseIntField(json, "population")) {
            config.populationSize = *value;
        }
        if (auto value = parseDoubleField(json, "mutation")) {
            config.mutationRate = *value;
        }
        if (auto value = parseDoubleField(json, "complexity_penalty")) {
            config.complexityPenalty = *value;
        }
        if (auto value = parseStringField(json, "format")) {
            config.outputFormat = *value;
        }
        if (auto value = parseStringField(json, "output")) {
            config.outputFile = *value;
        }
        if (auto value = parseIntField(json, "seed")) {
            config.seed = static_cast<unsigned int>(*value);
            config.useSeed = true;
        }
        if (auto value = parseBoolField(json, "use_seed")) {
            config.useSeed = *value;
        }
        if (auto value = parseBoolField(json, "evolve")) {
            config.evolve = *value;
        }
        if (auto value = parseBoolField(json, "verbose")) {
            config.verbose = *value;
        }
        if (auto value = parseBoolField(json, "scb_enabled")) {
            config.scbEnabled = *value;
        }
        if (auto value = parseIntMapField(json, "scb_costs")) {
            config.scbCosts = *value;
        }
    }
}