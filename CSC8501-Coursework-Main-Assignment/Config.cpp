#include "Config.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <string_view>

namespace ipd {
    namespace {
        bool startsWith(std::string_view value, std::string_view prefix) {
            return value.substr(0, prefix.size()) == prefix;
        }

        std::string_view trim(std::string_view value) {
            while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front()))) {
                value.remove_prefix(1);
            }

            while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back()))) {
                value.remove_suffix(1);
            }
            return value;
        }
    }

    Config Config::fromCommandLine(int argc, char** argv) {
        Config config;

        for (int i = 1; i < argc; ++i) {
            std::string_view argument(argv[i]);
            if (startsWith(argument, "--rounds=")) {
                config.rounds = std::atoi(std::string(argument.substr(9)).c_str());
            }
            else if (startsWith(argument, "--repeats=")) {
                config.repeats = std::atoi(std::string(argument.substr(10)).c_str());
            }
            else if (startsWith(argument, "--noise=")) {
                config.noise = std::atof(std::string(argument.substr(8)).c_str());
            }
            else if (startsWith(argument, "--seed=")) {
                config.seed = static_cast<unsigned int>(std::strtoul(std::string(argument.substr(7)).c_str(), nullptr, 10));
                config.useSeed = true;
            }
            else if (startsWith(argument, "--strategies=")) {
                config.strategyNames.clear();
                std::string names(argument.substr(13));
                std::size_t begin = 0;
                while (begin < names.size()) {
                    std::size_t comma = names.find(',', begin);
                    std::string token = names.substr(begin, comma == std::string::npos ? std::string::npos : comma - begin);
                    token = std::string(trim(token));
                    if (!token.empty()) {
                        config.strategyNames.push_back(token);
                    }
                    if (comma == std::string::npos) {
                        break;
                    }
                    begin = comma + 1;
                }
            }
            else if (startsWith(argument, "--generations=")) {
                config.generations = std::atoi(std::string(argument.substr(14)).c_str());
            }
            else if (startsWith(argument, "--population=")) {
                config.populationSize = std::atoi(std::string(argument.substr(13)).c_str());
            }
            else if (startsWith(argument, "--mutation=")) {
                config.mutationRate = std::atof(std::string(argument.substr(11)).c_str());
            }
            else if (startsWith(argument, "--penalty=")) {
                config.complexityPenalty = std::atof(std::string(argument.substr(10)).c_str());
            }
            else if (startsWith(argument, "--format=")) {
                config.outputFormat = std::string(trim(argument.substr(9)));
            }
            else if (startsWith(argument, "--output=")) {
                config.outputFile = std::string(trim(argument.substr(9)));
            }
            else if (argument == "--verbose") {
                config.verbose = true;
            }
            else if (startsWith(argument, "--payoff=")) {
                std::string payload(argument.substr(10));
                double values[4] = { config.payoff.T, config.payoff.R, config.payoff.P, config.payoff.S };
                std::size_t begin = 0;
                for (int index = 0; index < 4 && begin < payload.size(); ++index) {
                    std::size_t comma = payload.find(',', begin);
                    std::string token = payload.substr(begin, comma == std::string::npos ? std::string::npos : comma - begin);
                    values[index] = std::atof(token.c_str());
                    if (comma == std::string::npos) {
                        begin = payload.size();
                    }
                    else {
                        begin = comma + 1;
                    }
                }
                config.payoff = Payoff(values[0], values[1], values[2], values[3]);
            }
            else {
                throw std::runtime_error("Unknown command line argument: " + std::string(argument));
            }
        }

        config.ensureDefaults();
        return config;
    }

    void Config::ensureDefaults()
    {
        if (rounds <= 0) {
            rounds = 1;
        }
        if (repeats <= 0) {
            repeats = 1;
        } 
        if (noise < 0.0) {
            noise = 0.0;
        }
        else if (noise > 1.0) {
            noise = 1.0;
        }
        if (populationSize < 0) {
            populationSize = 0;
        }
        if (mutationRate < 0.0) {
            mutationRate = 0.0;
        }
        if (mutationRate > 1.0) {
            mutationRate = 1.0;
        }
        if (complexityPenalty < 0.0) {
            complexityPenalty = 0.0;
        }
        if (strategyNames.empty()) {
            strategyNames = { "ALLC", "ALLD", "TFT", "GRIM", "PAVLOV", "RND", "CTFT", "PROBER" };
        }
        if (populationSize == 0 && generations > 0) {
            populationSize = static_cast<int>(strategyNames.size());
        }
    }
}