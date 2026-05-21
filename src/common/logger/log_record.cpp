#include "log_record.h"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <mutex>

namespace AIstudy {
namespace common {
namespace logger {

namespace {

std::string levelString(LogLevel l) {
    switch (l) {
        case LogLevel::TRACE:   return "TRACE";
        case LogLevel::DEBUG:   return "DEBUG";
        case LogLevel::INFO:    return "INFO";
        case LogLevel::WARNING: return "WARN";
        case LogLevel::ERROR:   return "ERROR";
        case LogLevel::FATAL:   return "FATAL";
    }
    return "INFO";
}

std::string formatTime(std::chrono::system_clock::time_point tp) {
    auto t = std::chrono::system_clock::to_time_t(tp);
    std::tm tm_buf;
#if defined(_WIN32) || defined(_WIN64)
    gmtime_s(&tm_buf, &t);
    std::tm* tm = &tm_buf;
#else
    std::tm* tm = std::gmtime(&t);
#endif
    if (!tm) return {};
    std::ostringstream oss;
    oss << std::put_time(tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

std::vector<std::shared_ptr<IAppender>> s_appenders;
LogFilter s_filter;
bool s_filter_set = false;
std::mutex s_appender_mutex;

} // namespace

std::string LogRecord::toString() const {
    std::ostringstream oss;
    oss << formatTime(timestamp) << " [" << levelString(level) << "]";
    if (!module.empty()) oss << " [" << module << "]";
    if (!file.empty() && line > 0) oss << " [" << file << ":" << line << "]";
    oss << " - " << message;
    return oss.str();
}

void BufferAppender::append(const LogRecord& r) {
    records_.push_back(r);
}

void BufferAppender::clear() {
    records_.clear();
}

bool LogFilter::accept(const LogRecord& r) const {
    if (static_cast<int>(r.level) < static_cast<int>(minLevel))
        return false;
    if (!module.empty() && r.module != module)
        return false;
    return true;
}

void addLogAppender(std::shared_ptr<IAppender> a) {
    if (!a) return;
    std::lock_guard<std::mutex> lock(s_appender_mutex);
    s_appenders.push_back(std::move(a));
}

void clearLogAppenders() {
    std::lock_guard<std::mutex> lock(s_appender_mutex);
    s_appenders.clear();
}

void setLogFilter(LogFilter f) {
    std::lock_guard<std::mutex> lock(s_appender_mutex);
    s_filter = std::move(f);
    s_filter_set = true;
}

void emitToAppenders(const LogRecord& r) {
    std::vector<std::shared_ptr<IAppender>> copy;
    {
        std::lock_guard<std::mutex> lock(s_appender_mutex);
        if (s_filter_set && !s_filter.accept(r))
            return;
        copy = s_appenders;
    }
    for (const auto& a : copy)
        a->append(r);
}

} // namespace logger
} // namespace common
} // namespace AIstudy
