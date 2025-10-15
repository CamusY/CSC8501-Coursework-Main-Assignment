#pragma once

#include <memory>
#include <vector>

#include "Config.h"
#include "Result.h"

namespace ipd {
    class Reporter {
    public:
        virtual ~Reporter() = default;

        virtual void reportTournament(const Config& config, const std::vector<Result>& results) = 0;
        virtual void reportEvolution(const Config& config, const std::vector<Result>& results) = 0;
    };

    using ReporterPtr = std::unique_ptr<Reporter>;

    ReporterPtr makeConsoleReporter();
    ReporterPtr makeCsvReporter(const std::string& path);
}