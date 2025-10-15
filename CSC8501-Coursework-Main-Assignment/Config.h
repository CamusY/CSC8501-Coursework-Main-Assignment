#pragma once

#include <string>
#include <vector>
#include "Payoff.h"

namespace ipd {
    struct Config {
        int rounds = 200;
        int repeats = 1;
        double noise = 0.0;
        unsigned int seed = 0;
        bool useSeed = false;
        Payoff payoff;
        std::vector<std::string> strategyNames;
        int generations = 0;
        int populationSize = 0;
        double mutationRate = 0.0;
        double complexityPenalty = 0.0;
        std::string outputFormat = "text";
        std::string outputFile = "";
        bool verbose = false;

        static Config fromCommandLine(int argc, char** argv);
        void ensureDefaults();
    };
}