#include <exception>
#include <iostream>
#include <vector>

#include "Config.h"
#include "EvolutionManager.h"
#include "Reporter.h"
#include "TournamentManager.h"
#include "Logger.h"

int main(int argc, char** argv) {
    try {
        ipd::Config config = ipd::Config::fromCommandLine(argc, argv);

        if (config.verbose) {
            ipd::Logger::instance().setEnabled(true);
        }

        std::vector<ipd::Result> results;
        std::vector<ipd::GenerationShare> history;

        if (config.evolve || config.generations > 0) {
            ipd::EvolutionManager evolution;
            ipd::EvolutionOutcome outcome = evolution.run(config);
            results = std::move(outcome.results);
            history = std::move(outcome.history);
            ipd::writeEvolutionSharesCsv(config, history);
        }
        else {
            ipd::TournamentManager tournament;
            results = tournament.run(config);
        }

        ipd::reportResults(config, results, history);

        if (!config.saveFile.empty()) {
            config.saveToJson(config.saveFile);
        }
    }
    catch (const std::exception& ex) {
        std::cerr << "error: " << ex.what() << '\n';
        return 1;
    }

    return 0;
}
