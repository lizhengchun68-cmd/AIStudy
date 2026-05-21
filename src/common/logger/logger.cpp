#include "logger.h"
#include "logger_config.h"
#include "log_record.h"
#include <Poco/Logger.h>
#include <Poco/Message.h>
#include <Poco/Util/LoggingConfigurator.h>
#include <Poco/Util/PropertyFileConfiguration.h>
#include <chrono>
#include <mutex>

namespace AIstudy {
namespace common {
namespace logger {

namespace {

std::recursive_mutex s_logger_mutex;
bool s_configured = false;

int levelToPoco(LogLevel l) {
    switch (l) {
        case LogLevel::TRACE:   return Poco::Message::PRIO_TRACE;
        case LogLevel::DEBUG:   return Poco::Message::PRIO_DEBUG;
        case LogLevel::INFO:    return Poco::Message::PRIO_INFORMATION;
        case LogLevel::WARNING: return Poco::Message::PRIO_WARNING;
        case LogLevel::ERROR:   return Poco::Message::PRIO_ERROR;
        case LogLevel::FATAL:   return Poco::Message::PRIO_FATAL;
    }
    return Poco::Message::PRIO_INFORMATION;
}

LogLevel pocoToLevel(int p) {
    switch (p) {
        case Poco::Message::PRIO_TRACE:       return LogLevel::TRACE;
        case Poco::Message::PRIO_DEBUG:       return LogLevel::DEBUG;
        case Poco::Message::PRIO_INFORMATION: return LogLevel::INFO;
        case Poco::Message::PRIO_WARNING:     return LogLevel::WARNING;
        case Poco::Message::PRIO_ERROR:       return LogLevel::ERROR;
        case Poco::Message::PRIO_CRITICAL:
        case Poco::Message::PRIO_FATAL:       return LogLevel::FATAL;
        default:                              return LogLevel::INFO;
    }
}

} // namespace

Logger Logger::get(const std::string& name) {
    return Logger(name);
}

Logger Logger::defaultLogger() {
    return get("SimUtils");
}

void Logger::initialize() {
    std::lock_guard<std::recursive_mutex> lock(s_logger_mutex);
    if (!s_configured)
        setupDefaultConfiguration();
}

void Logger::shutdown() {
    std::lock_guard<std::recursive_mutex> lock(s_logger_mutex);
    Poco::Logger::shutdown();
    s_configured = false;
}

void Logger::configureFromFile(const std::string& path) {
    std::lock_guard<std::recursive_mutex> lock(s_logger_mutex);
    auto cfg = new Poco::Util::PropertyFileConfiguration(path);
    Poco::Util::AbstractConfiguration::Ptr p(cfg);
    Poco::Util::LoggingConfigurator().configure(p);
    s_configured = true;
}

void Logger::setupDefaultConfiguration() {
    std::lock_guard<std::recursive_mutex> lock(s_logger_mutex);
    applyLoggerConfig(LoggerConfig());
    s_configured = true;
}

int Logger::toPocoPriority(LogLevel l) const {
    return levelToPoco(l);
}

LogLevel Logger::fromPocoPriority(int p) const {
    return pocoToLevel(p);
}

void Logger::setLevel(LogLevel level) {
    Poco::Logger::get(name_).setLevel(levelToPoco(level));
}

LogLevel Logger::getLevel() const {
    return pocoToLevel(Poco::Logger::get(name_).getLevel());
}

bool Logger::isLevelEnabled(LogLevel level) const {
    return Poco::Logger::get(name_).is(levelToPoco(level));
}

namespace {
void emitRecord(LogLevel level, const std::string& name, const std::string& msg,
                const char* file, int line) {
    LogRecord r;
    r.timestamp = std::chrono::system_clock::now();
    r.level = level;
    r.module = name;
    r.file = file ? file : "";
    r.line = line;
    r.message = msg;
    emitToAppenders(r);
}
}  // namespace

void Logger::trace(const std::string& msg) {
    Poco::Logger::get(name_).trace(msg);
    emitRecord(LogLevel::TRACE, name_, msg, nullptr, 0);
}
void Logger::debug(const std::string& msg) {
    Poco::Logger::get(name_).debug(msg);
    emitRecord(LogLevel::DEBUG, name_, msg, nullptr, 0);
}
void Logger::info(const std::string& msg) {
    Poco::Logger::get(name_).information(msg);
    emitRecord(LogLevel::INFO, name_, msg, nullptr, 0);
}
void Logger::warning(const std::string& msg) {
    Poco::Logger::get(name_).warning(msg);
    emitRecord(LogLevel::WARNING, name_, msg, nullptr, 0);
}
void Logger::error(const std::string& msg) {
    Poco::Logger::get(name_).error(msg);
    emitRecord(LogLevel::ERROR, name_, msg, nullptr, 0);
}
void Logger::fatal(const std::string& msg) {
    Poco::Logger::get(name_).fatal(msg);
    emitRecord(LogLevel::FATAL, name_, msg, nullptr, 0);
}

void Logger::trace(const std::string& msg, const char* file, int line) {
    Poco::Logger::get(name_).trace(msg, file, line);
    emitRecord(LogLevel::TRACE, name_, msg, file, line);
}
void Logger::debug(const std::string& msg, const char* file, int line) {
    Poco::Logger::get(name_).debug(msg, file, line);
    emitRecord(LogLevel::DEBUG, name_, msg, file, line);
}
void Logger::info(const std::string& msg, const char* file, int line) {
    Poco::Logger::get(name_).information(msg, file, line);
    emitRecord(LogLevel::INFO, name_, msg, file, line);
}
void Logger::warning(const std::string& msg, const char* file, int line) {
    Poco::Logger::get(name_).warning(msg, file, line);
    emitRecord(LogLevel::WARNING, name_, msg, file, line);
}
void Logger::error(const std::string& msg, const char* file, int line) {
    Poco::Logger::get(name_).error(msg, file, line);
    emitRecord(LogLevel::ERROR, name_, msg, file, line);
}
void Logger::fatal(const std::string& msg, const char* file, int line) {
    Poco::Logger::get(name_).fatal(msg, file, line);
    emitRecord(LogLevel::FATAL, name_, msg, file, line);
}

} // namespace logger
} // namespace common
} // namespace AIstudy
