#include "e46/common/logging.h"
#include <cstdio>
#include <cstdarg>
#include <ctime>
#include <mutex>
#include <fcntl.h>
#include <unistd.h>

namespace e46 {
namespace common {

Logger& Logger::instance()
{
    static Logger inst;
    return inst;
}

Logger::~Logger()
{
    if (fileFd_ >= 0) {
        close(fileFd_);
    }
    if (useSyslog_) {
        closelog();
    }
}

void Logger::init(const std::string& ident, bool useSyslog, const std::string& logFile)
{
    if (useSyslog) {
        openlog(ident.c_str(), LOG_PID | LOG_NDELAY, LOG_DAEMON);
        useSyslog_ = true;
    } else {
        useSyslog_ = false;
    }

    if (!logFile.empty()) {
        fileFd_ = open(logFile.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
    }
}

// va_list overload — single format pass, avoids double-formatting in
// the convenience methods (debug/info/warn/error).
void Logger::log(LogLevel level, const char* fmt, va_list args)
{
    if (level < minLevel_) return;

    // Format message once
    char buf[1024];
    vsnprintf(buf, sizeof(buf), fmt, args);

    // Timestamp prefix
    time_t now = time(nullptr);
    struct tm tm_buf;
    localtime_r(&now, &tm_buf);
    char timeStr[32];
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &tm_buf);

    // Protect multi-threaded writes to file / stderr
    static std::mutex logMutex;
    std::lock_guard<std::mutex> lock(logMutex);

    if (useSyslog_) {
        syslog(toSyslogPriority(level), "%s", buf);
    }

    if (fileFd_ >= 0) {
        dprintf(fileFd_, "[%s] [%s] %s\n", timeStr, levelString(level), buf);
    }

    // Also to stderr for foreground debugging (atomic under the mutex)
    fprintf(stderr, "[%s] [%s] %s\n", timeStr, levelString(level), buf);
}

// Public variadic entry point — delegates to va_list overload
void Logger::log(LogLevel level, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    log(level, fmt, args);
    va_end(args);
}

void Logger::debug(const char* fmt, ...) {
    va_list args; va_start(args, fmt);
    log(LogLevel::DEBUG, fmt, args);
    va_end(args);
}
void Logger::info(const char* fmt, ...) {
    va_list args; va_start(args, fmt);
    log(LogLevel::INFO, fmt, args);
    va_end(args);
}
void Logger::warn(const char* fmt, ...) {
    va_list args; va_start(args, fmt);
    log(LogLevel::WARN, fmt, args);
    va_end(args);
}
void Logger::error(const char* fmt, ...) {
    va_list args; va_start(args, fmt);
    log(LogLevel::ERROR, fmt, args);
    va_end(args);
}

int Logger::toSyslogPriority(LogLevel level) const
{
    switch (level) {
        case LogLevel::DEBUG: return LOG_DEBUG;
        case LogLevel::INFO:  return LOG_INFO;
        case LogLevel::WARN:  return LOG_WARNING;
        case LogLevel::ERROR: return LOG_ERR;
        default: return LOG_INFO;
    }
}

const char* Logger::levelString(LogLevel level) const
{
    switch (level) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO:  return "INFO";
        case LogLevel::WARN:  return "WARN";
        case LogLevel::ERROR: return "ERROR";
        default: return "????";
    }
}

} // namespace common
} // namespace e46
