#pragma once

#include <iostream>
#include <mutex>
#include <string>

namespace ipd {
    enum class LogLevel {
        Debug,
        Info,
        Warning,
        Error
    };

    class Logger {
    public:
        static Logger& instance();

        void setEnabled(bool enabled);
        void setLevel(LogLevel level);

        void log(LogLevel level, const std::string& message);

    private:
        Logger() = default;
        std::mutex m_mutex;
        bool m_enabled = false;
        LogLevel m_level = LogLevel::Info;
    };

    void logDebug(const std::string& message);
    void logInfo(const std::string& message);
    void logWarning(const std::string& message);
    void logError(const std::string& message);
}
