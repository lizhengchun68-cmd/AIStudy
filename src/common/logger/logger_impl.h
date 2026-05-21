#ifndef COMMON_LOGGER_LOGGER_IMPL_H
#define COMMON_LOGGER_LOGGER_IMPL_H

#include "logger.h"
#include <Poco/Format.h>

namespace AIstudy {
namespace common {
namespace logger {

template <typename T, typename... Args>
void Logger::trace(const std::string& fmt, T arg1, Args&&... args) {
    trace(Poco::format(fmt, arg1, std::forward<Args>(args)...));
}
template <typename T, typename... Args>
void Logger::debug(const std::string& fmt, T arg1, Args&&... args) {
    debug(Poco::format(fmt, arg1, std::forward<Args>(args)...));
}
template <typename T, typename... Args>
void Logger::info(const std::string& fmt, T arg1, Args&&... args) {
    info(Poco::format(fmt, arg1, std::forward<Args>(args)...));
}
template <typename T, typename... Args>
void Logger::warning(const std::string& fmt, T arg1, Args&&... args) {
    warning(Poco::format(fmt, arg1, std::forward<Args>(args)...));
}
template <typename T, typename... Args>
void Logger::error(const std::string& fmt, T arg1, Args&&... args) {
    error(Poco::format(fmt, arg1, std::forward<Args>(args)...));
}
template <typename T, typename... Args>
void Logger::fatal(const std::string& fmt, T arg1, Args&&... args) {
    fatal(Poco::format(fmt, arg1, std::forward<Args>(args)...));
}

} // namespace logger
} // namespace common
} // namespace AIstudy

#endif // SIMUTILS_LOGGER_LOGGER_IMPL_H
