#include "Statistics.h"

#include <cmath>
#include <numeric>

namespace ipd {
    namespace statistics {
        double mean(const std::vector<double>& values) {
            if (values.empty()) {
                return 0.0;
            }
            const double sum = std::accumulate(values.begin(), values.end(), 0.0);
            return sum / static_cast<double>(values.size());
        }

        double variance(const std::vector<double>& values, double meanValue) {
            if (values.size() < 2) {
                return 0.0;
            }
            const double sumOfSquares = std::accumulate(values.begin(), values.end(), 0.0, [meanValue](double total, double value) {
                const double diff = value - meanValue;
                return total + diff * diff;
                });
            return sumOfSquares / static_cast<double>(values.size() - 1);
        }

        std::tuple<double, double> confidenceInterval95(const std::vector<double>& values, double meanValue) {
            if (values.size() < 2) {
                return { meanValue, meanValue };
            }
            const double var = variance(values, meanValue);
            const double stdev = std::sqrt(var);
            const double standardError = stdev / std::sqrt(static_cast<double>(values.size()));
            const double margin = 1.96 * standardError;
            return { meanValue - margin, meanValue + margin };
        }
    }
}