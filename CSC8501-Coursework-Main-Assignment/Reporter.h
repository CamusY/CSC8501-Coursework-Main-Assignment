#pragma once

#include <vector>

#include "Config.h"
#include "EvolutionManager.h"
#include "Result.h"

namespace ipd {
    void reportResults(const Config& config, const std::vector<Result>& results, const std::vector<GenerationShare>& history);
}