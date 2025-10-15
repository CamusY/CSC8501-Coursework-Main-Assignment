#include <exception>
#include <iostream>
#include <memory>
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
        if (config.generations > 0) {
            ipd::EvolutionManager evolution;
            results = evolution.run(config);
        }
        else {
            ipd::TournamentManager tournament;
            results = tournament.run(config);
        }

        std::vector<ipd::ReporterPtr> reporters;
        reporters.push_back(ipd::makeConsoleReporter());
        if (config.outputFormat == "csv" || config.outputFormat == "both") {
            reporters.push_back(ipd::makeCsvReporter(config.outputFile));
        }

        for (auto& reporter : reporters) {
            if (config.generations > 0) {
                reporter->reportEvolution(config, results);
            }
            else {
                reporter->reportTournament(config, results);
            }
        }
    }
    catch (const std::exception& ex) {
        std::cerr << "error: " << ex.what() << '\n';
        return 1;
    }

    return 0;
}
