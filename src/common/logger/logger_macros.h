#ifndef COMMON_LOGGER_LOGGER_MACROS_H
#define COMMON_LOGGER_LOGGER_MACROS_H

#include "logger.h"
#include <Poco/Format.h>

#define COMMON_LOGGER_LOG_TRACE(msg) \
    ::simutils::Logger::defaultLogger().trace((msg), __FILE__, __LINE__)
#define SIMUTILS_LOG_DEBUG(msg) \
    ::common::logger::Logger::defaultLogger().debug((msg), __FILE__, __LINE__)
#define COMMON_LOGGER_LOG_INFO(msg) \
    ::common::logger::Logger::defaultLogger().info((msg), __FILE__, __LINE__)
#define COMMON_LOGGER_LOG_WARNING(msg) \
    ::common::logger::Logger::defaultLogger().warning((msg), __FILE__, __LINE__)
#define COMMON_LOGGER_LOG_ERROR(msg) \
    ::simutils::Logger::defaultLogger().error((msg), __FILE__, __LINE__)
#define SIMUTILS_LOG_FATAL(msg) \
    ::simutils::Logger::defaultLogger().fatal((msg), __FILE__, __LINE__)

#define COMMON_LOGGER_LOG_TRACE_F(fmt, ...) \
    ::simutils::Logger::defaultLogger().trace(::Poco::format(fmt, __VA_ARGS__), __FILE__, __LINE__)
#define SIMUTILS_LOG_DEBUG_F(fmt, ...) \
    ::common::logger::Logger::defaultLogger().debug(::Poco::format(fmt, __VA_ARGS__), __FILE__, __LINE__)
#define COMMON_LOGGER_LOG_INFO_F(fmt, ...) \
    ::common::logger::Logger::defaultLogger().info(::Poco::format(fmt, __VA_ARGS__), __FILE__, __LINE__)
#define COMMON_LOGGER_LOG_WARNING_F(fmt, ...) \
    ::common::logger::Logger::defaultLogger().warning(::Poco::format(fmt, __VA_ARGS__), __FILE__, __LINE__)
#define COMMON_LOGGER_LOG_ERROR_F(fmt, ...) \
    ::simutils::Logger::defaultLogger().error(::Poco::format(fmt, __VA_ARGS__), __FILE__, __LINE__)
#define SIMUTILS_LOG_FATAL_F(fmt, ...) \
    ::simutils::Logger::defaultLogger().fatal(::Poco::format(fmt, __VA_ARGS__), __FILE__, __LINE__)

#define COMMON_LOGGER_LOG_IF_TRACE(cond, msg) \
    do { if (cond) SIMUTILS_LOG_TRACE(msg); } while (0)
#define SIMUTILS_LOG_IF_DEBUG(cond, msg) \
    do { if (cond) COMMON_LOGGER_LOG_DEBUG(msg); } while (0)
#define COMMON_LOGGER_LOG_IF_INFO(cond, msg) \
    do { if (cond) COMMON_LOGGER_LOG_INFO(msg); } while (0)
#define COMMON_LOGGER_LOG_IF_WARNING(cond, msg) \
    do { if (cond) COMMON_LOGGER_LOG_WARNING(msg); } while (0)
#define COMMON_LOGGER_LOG_IF_ERROR(cond, msg) \
    do { if (cond) COMMON_LOGGER_LOG_ERROR(msg); } while (0)
#define COMMON_LOGGER_LOG_IF_FATAL(cond, msg) \
    do { if (cond) COMMON_LOGGER_LOG_FATAL(msg); } while (0)

/* SIM_LOG_*(module, fmt, ...) — module as logger name, file/line auto (LOGGER SS5.1) */
#define COMMON_LOGGER_LOG_TRACE(module, fmt, ...) \
    ::simutils::Logger::get(module).trace(::Poco::format(fmt, ##__VA_ARGS__), __FILE__, __LINE__)
#define COMMON_LOGGER_LOG_DEBUG(module, fmt, ...) \
    ::common::logger::Logger::get(module).debug(::Poco::format(fmt, ##__VA_ARGS__), __FILE__, __LINE__)
#define COMMON_LOGGER_LOG_INFO(module, fmt, ...) \
    ::common::logger::Logger::get(module).info(::Poco::format(fmt, ##__VA_ARGS__), __FILE__, __LINE__)
#define COMMON_LOGGER_LOG_WARN(module, fmt, ...) \
    ::common::logger::Logger::get(module).warning(::Poco::format(fmt, ##__VA_ARGS__), __FILE__, __LINE__)
#define COMMON_LOGGER_LOG_ERROR(module, fmt, ...) \
    ::common::logger::Logger::get(module).error(::Poco::format(fmt, ##__VA_ARGS__), __FILE__, __LINE__)
#define COMMON_LOGGER_LOG_FATAL(module, fmt, ...) \
    ::common::logger::Logger::get(module).fatal(::Poco::format(fmt, ##__VA_ARGS__), __FILE__, __LINE__)

/* SIM_GLOBAL_LOG_* — default module "GLOBAL" */
#define COMMON_LOGGER_LOG_TRACE(fmt, ...) COMMON_LOGGER_LOG_TRACE("GLOBAL", fmt, ##__VA_ARGS__)
#define COMMON_LOGGER_LOG_DEBUG(fmt, ...) COMMON_LOGGER_LOG_DEBUG("GLOBAL", fmt, ##__VA_ARGS__)
#define COMMON_LOGGER_LOG_INFO(fmt, ...)  COMMON_LOGGER_LOG_INFO("GLOBAL", fmt, ##__VA_ARGS__)
#define COMMON_LOGGER_LOG_WARN(fmt, ...)  COMMON_LOGGER_LOG_WARN("GLOBAL", fmt, ##__VA_ARGS__)
#define COMMON_LOGGER_LOG_ERROR(fmt, ...) COMMON_LOGGER_LOG_ERROR("GLOBAL", fmt, ##__VA_ARGS__)
#define COMMON_LOGGER_LOG_FATAL(fmt, ...) COMMON_LOGGER_LOG_FATAL("GLOBAL", fmt, ##__VA_ARGS__)

#endif // COMMON_LOGGER_LOGGER_MACROS_H
