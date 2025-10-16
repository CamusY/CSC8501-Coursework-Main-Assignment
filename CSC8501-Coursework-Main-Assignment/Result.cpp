#include "Result.h"

#include <iomanip>
#include <sstream>

namespace ipd {
    std::string Result::toString() const {
        std::ostringstream stream;
        stream << strategy << ": mean=" << std::fixed << std::setprecision(3) << mean;
        stream << ", stdev=" << std::fixed << std::setprecision(3) << stdev;
        stream << ", CI95=[" << std::fixed << std::setprecision(3) << ciLow << ", " << ciHigh << "]";
        stream << ", coopRate=" << std::fixed << std::setprecision(3) << coopRate;
        stream << ", firstDefection=" << std::fixed << std::setprecision(3) << firstDefection;
        stream << ", echoLength=" << std::fixed << std::setprecision(3) << echoLength;
        stream << ", complexity=" << std::fixed << std::setprecision(3) << complexity;
        if (samples > 0) {
            stream << ", samples=" << samples;
        }
        if (extra != 0.0) {
            stream << ", extra=" << std::fixed << std::setprecision(3) << extra;
        }
        return stream.str();
    }

    std::string Result::toCsv() const {
        std::ostringstream stream;
        stream << '"' << strategy << '"';
        stream << ',' << mean << ',' << stdev << ',' << ciLow << ',' << ciHigh << ','
            << coopRate << ',' << firstDefection << ',' << echoLength << ','
            << complexity << ',' << samples << ',' << extra;
        return stream.str();
    }
}