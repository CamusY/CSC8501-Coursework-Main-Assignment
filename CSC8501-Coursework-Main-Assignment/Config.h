#pragma once

#include <optional>
#include <string>
#include <vector>
#include <unordered_map>

#include "Payoff.h"

namespace ipd {
    struct Config {
        int rounds = 200;
        int repeats = 1;
        double epsilon = 0.0;
        unsigned int seed = 0;
        bool useSeed = false;
        Payoff payoffs;
        std::vector<std::string> strategyNames;
        int generations = 0;
        int populationSize = 0;
        double mutationRate = 0.0;
        double complexityPenalty = 0.0;
        std::string outputFormat = "text";
        std::string outputFile = "";
        bool verbose = false;
		bool evolve = false;
        std::string saveFile;
		std::string loadFile;
        bool scbEnabled = false;
        std::unordered_map<std::string, int> scbCosts;

        static Config fromCommandLine(int argc, char** argv);
        void ensureDefaults();
		void saveToJson(const std::string& filename) const;
		static void loadFromJson(const std::string& path, Config& config);
    };
}