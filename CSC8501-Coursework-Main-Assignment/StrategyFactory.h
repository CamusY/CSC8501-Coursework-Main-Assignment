#pragma once

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

#include "Strategy.h"

namespace ipd {
    class StrategyFactory {
    public:
        using Creator = std::function<StrategyPtr()>;

        static StrategyFactory& instance();

        StrategyPtr create(const std::string& name) const;
        bool hasStrategy(const std::string& name) const;
        void registerStrategy(const std::string& name, Creator creator);
        std::vector<std::string> availableStrategies() const;

    private:
        StrategyFactory() = default;

        std::unordered_map<std::string, Creator> m_creators;
    };

    void registerBuiltinStrategies();
}