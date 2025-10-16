#pragma once

#include <cstddef>
#include <string>

namespace ipd {
    struct Result {
        std::string strategy;
        double mean = 0.0;
        double variance = 0.0;
        double stdev = 0.0;
        double ciLow = 0.0;
        double ciHigh = 0.0;
		double coopRate = 0.0;
        double firstDefection = 0.0;
        double echoLength = 0.0;
        double complexity = 0.0;
        std::size_t samples = 0;
        double extra = 0.0; // generic field used by evolution frequency etc.

        std::string toString() const;
        std::string toCsv() const;
    };
}