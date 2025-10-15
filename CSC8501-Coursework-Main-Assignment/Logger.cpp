#include "Logger.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace ipd {
    namespace {
        std::string formatPrefix(LogLevel level) {
            switch (level) {
            case LogLevel::Debug:
                return "[DEBUG]";
            case LogLevel::Info:
                return "[INFO ]";
            case LogLevel::Warning:
                return "[WARN ]";
            case LogLevel::Error:
            default:
                return "[ERROR]";
            }
        }
    }

    Logger& Logger::instance() {
        static Logger instance;
        return instance;
    }

    void Logger::setEnabled(bool enabled) {
        m_enabled = enabled;
    }

    void Logger::setLevel(LogLevel level) {
        m_level = level;
    }

    void Logger::log(LogLevel level, const std::string& message) {
        if (!m_enabled || level < m_level) {
            return;
        }

        std::lock_guard<std::mutex> lock(m_mutex);
        std::ostringstream stream;
        const auto now = std::chrono::system_clock::now();
        const auto time = std::chrono::system_clock::to_time_t(now);
        stream << formatPrefix(level) << ' ';
        std::tm tm_buf;
        #if defined(_MSC_VER)
            localtime_s(&tm_buf, &time);
        #else
            localtime_r(&time, &tm_buf);
        #endif
            stream << std::put_time(&tm_buf, "%F %T") << " - " << message << '\n';
        std::cerr << stream.str();
    }

    void logDebug(const std::string& message) {
        Logger::instance().log(LogLevel::Debug, message);
    }

    void logInfo(const std::string& message) {
        Logger::instance().log(LogLevel::Info, message);
    }

    void logWarning(const std::string& message) {
        Logger::instance().log(LogLevel::Warning, message);
    }

    void logError(const std::string& message) {
        Logger::instance().log(LogLevel::Error, message);
    }
}
