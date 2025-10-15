#include "StrategyFactory.h"

#include <algorithm>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>

#include "ALLC.h"
#include "ALLD.h"
#include "CTFT.h"
#include "Empath.h"
#include "Reflector.h"
#include "GRIM.h"
#include "PAVLOV.h"
#include "PROBER.h"
#include "RND.h"
#include "TFT.h"

namespace ipd {
    namespace {
        std::optional<double> parseRandomProbability(const std::string& name) {
            if (name == "RND") {
                return 0.5;
            }
            if (name.size() <= 5 || name.substr(0, 4) != "RND(") {
                return std::nullopt;
            }
            if (name.back() != ')') {
                throw std::runtime_error("Invalid RND strategy format: " + name);
            }
            const std::string parameter = name.substr(4, name.size() - 5);
            std::stringstream stream(parameter);
            double probability = 0.5;
            stream >> probability;
            if (!stream || !stream.eof()) {
                throw std::runtime_error("Invalid probability for RND strategy: " + name);
            }
            if (probability < 0.0 || probability > 1.0) {
                throw std::runtime_error("RND probability must be between 0 and 1: " + name);
            }
            return probability;
        }
    }

    StrategyFactory& StrategyFactory::instance() {
        static StrategyFactory factory;
        return factory;
    }

    StrategyPtr StrategyFactory::create(const std::string& name) const {
        if (const auto probability = parseRandomProbability(name)) {
            return std::make_unique<RND>(*probability);
        }
        auto it = m_creators.find(name);
        if (it == m_creators.end()) {
            throw std::runtime_error("Unknown strategy: " + name);
        }
        return it->second();
    }

    bool StrategyFactory::hasStrategy(const std::string& name) const {
        return m_creators.find(name) != m_creators.end();
    }

    void StrategyFactory::registerStrategy(const std::string& name, Creator creator) {
        m_creators[name] = std::move(creator);
    }

    std::vector<std::string> StrategyFactory::availableStrategies() const {
        std::vector<std::string> names;
        names.reserve(m_creators.size());
        for (const auto& entry : m_creators)
        {
            names.push_back(entry.first);
        }
        std::sort(names.begin(), names.end());
        return names;
    }

    void registerBuiltinStrategies() {
        auto& factory = StrategyFactory::instance();
        factory.registerStrategy("ALLC", []() { return std::make_unique<ALLC>(); });
        factory.registerStrategy("ALLD", []() { return std::make_unique<ALLD>(); });
        factory.registerStrategy("TFT", []() { return std::make_unique<TFT>(); });
        factory.registerStrategy("GRIM", []() { return std::make_unique<GRIM>(); });
        factory.registerStrategy("PAVLOV", []() { return std::make_unique<PAVLOV>(); });
        factory.registerStrategy("RND", []() { return std::make_unique<RND>(0.5); });
        factory.registerStrategy("CTFT", []() { return std::make_unique<CTFT>(); });
        factory.registerStrategy("PROBER", []() { return std::make_unique<PROBER>(); });
        factory.registerStrategy("Empath", []() { return std::make_unique<Empath>(); });
        factory.registerStrategy("Reflector", []() { return std::make_unique<Reflector>(); });
    }
}