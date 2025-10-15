#include "Statistics.h"

#include <cmath>

namespace ipd {
    namespace statistics {
        double mean(const std::vector<double>& values) {
            if (values.empty()) {
                return 0.0;
            }

            double sum = 0.0;
            for (double value : values) {
                sum += value;
            }
            return sum / static_cast<double>(values.size());
        }

        double variance(const std::vector<double>& values, double meanValue) {
            if (values.size() < 2) {
                return 0.0;
            }

            double sum = 0.0;
            for (double value : values) {
                const double diff = value - meanValue;
                sum += diff * diff;
            }
            return sum / static_cast<double>(values.size() - 1);
        }

        std::tuple<double, double> confidenceInterval95(const std::vector<double>& values, double meanValue) {
            if (values.size() < 2) {
                return { meanValue, meanValue };
            }

            const double var = variance(values, meanValue);
            const double standardError = std::sqrt(var / static_cast<double>(values.size()));
            const double margin = 1.96 * standardError;
            return { meanValue - margin, meanValue + margin };
        }
    }
}
