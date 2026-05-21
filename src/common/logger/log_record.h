#ifndef COMMON_LOGGER_LOG_RECORD_H
#define COMMON_LOGGER_LOG_RECORD_H

#include "logger.h"
#include <chrono>
#include <memory>
#include <string>
#include <vector>

namespace AIstudy {
namespace common {
namespace logger {

/** @brief Structured log record (LOGGER §5). */
struct LogRecord {
    std::chrono::system_clock::time_point timestamp;
    LogLevel level = LogLevel::INFO;
    std::string module;
    std::string file;
    int line = 0;
    std::string message;

    /** @brief Format as single line (timestamp [LEVEL] [module] - message). */
    std::string toString() const;
};

/** @brief Appender base (LOGGER §5). */
class IAppender {
public:
    virtual ~IAppender() = default;
    virtual void append(const LogRecord& r) = 0;
};

/** @brief In-memory appender for tests (LOGGER §7.2). */
class BufferAppender : public IAppender {
public:
    void append(const LogRecord& r) override;
    void clear();
    const std::vector<LogRecord>& records() const { return records_; }

private:
    std::vector<LogRecord> records_;
};

/** @brief Filter by level and optional module (LOGGER §5). */
struct LogFilter {
    LogLevel minLevel = LogLevel::TRACE;
    std::string module;  /* empty = any */

    bool accept(const LogRecord& r) const;
};

/** @brief Add appender (global). */
void addLogAppender(std::shared_ptr<IAppender> a);
/** @brief Remove all appenders. */
void clearLogAppenders();
/** @brief Set global filter; empty optional = no filter. */
void setLogFilter(LogFilter f);
/** @brief Emit record to all appenders (respects filter). */
void emitToAppenders(const LogRecord& r);

} // namespace logger
} // namespace common
} // namespace AIstudy

#endif // COMMON_LOGGER_LOG_RECORD_H
