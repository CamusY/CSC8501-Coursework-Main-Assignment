#pragma once

#include <cstddef>
#include <tuple>
#include <vector>

namespace ipd {
    namespace statistics {
        double mean(const std::vector<double>& values);
        double variance(const std::vector<double>& values, double meanValue);
        std::tuple<double, double> confidenceInterval95(const std::vector<double>& values, double meanValue);
    }
}

