#pragma once

#include <string>
#include <memory>
#include <syslog.h>

namespace e46 {
namespace common {

enum class LogLevel { DEBUG, INFO, WARN, ERROR };

class Logger
{
public:
    static Logger& instance();

    void init(const std::string& ident = "e46",
              bool useSyslog = true,
              const std::string& logFile = "");

    // Public variadic entry point (used by macros)
    void log(LogLevel level, const char* fmt, ...);

    // Convenience methods
    void debug(const char* fmt, ...);
    void info(const char* fmt, ...);
    void warn(const char* fmt, ...);
    void error(const char* fmt, ...);

    void setLevel(LogLevel minLevel) { minLevel_ = minLevel; }

private:
    Logger() = default;
    ~Logger();

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    int toSyslogPriority(LogLevel level) const;
    const char* levelString(LogLevel level) const;

    bool useSyslog_ = true;
    LogLevel minLevel_ = LogLevel::DEBUG;
    int fileFd_ = -1;
};

// Convenience macros
#define E46_LOG_DEBUG(fmt, ...) e46::common::Logger::instance().debug(fmt, ##__VA_ARGS__)
#define E46_LOG_INFO(fmt, ...)  e46::common::Logger::instance().info(fmt, ##__VA_ARGS__)
#define E46_LOG_WARN(fmt, ...)  e46::common::Logger::instance().warn(fmt, ##__VA_ARGS__)
#define E46_LOG_ERROR(fmt, ...) e46::common::Logger::instance().error(fmt, ##__VA_ARGS__)

} // namespace common
} // namespace e46
