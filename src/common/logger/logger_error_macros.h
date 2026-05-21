#ifndef COMMON_LOGGER_LOGGER_ERROR_MACROS_H
#define COMMON_LOGGER_LOGGER_ERROR_MACROS_H

#include "logger_error.h"
#include <Poco/Format.h>

#define COMMON_LOGGER_LOG_ERROR_CODE(error, msg) \
    ::common::logger::logErrorCode((error), (msg))

#define COMMON_LOGGER_LOG_ERROR_CODE_F(error, fmt, ...) \
    do { \
        ::common::logger::ErrorCodeWrapper _ec = (error); \
        ::common::logger::logErrorCode(_ec, ::Poco::format(fmt, ##__VA_ARGS__)); \
    } while (0)

#define COMMON_LOGGER_LOG_EXCEPTION(e) \
    ::common::logger::logException(static_cast<const std::exception&>(e), __FILE__, __LINE__)

/** @brief Log StatusOr error when !ok() (LOGGER §3.2). */
#define COMMON_LOGGER_LOG_STATUSOR(res) ::common::logger::logStatusOr(res)

#define COMMON_LOGGER_LOG_STATUSOR_F(res, fmt, ...) \
    ::common::logger::logStatusOr((res), ::Poco::format(fmt, ##__VA_ARGS__))

/* Module-aware (LOGGER §5.1) */
#define COMMON_LOGGER_LOG_ERROR_CODE(module, code, fmt, ...) \
    ::common::logger::logErrorCode((module), (code), ::Poco::format(fmt, ##__VA_ARGS__), __FILE__, __LINE__)

#define COMMON_LOGGER_LOG_EXCEPTION(module, e) \
    ::common::logger::logException((module), static_cast<const std::exception&>(e), __FILE__, __LINE__, "")
#define COMMON_LOGGER_LOG_EXCEPTION_F(module, e, fmt, ...) \
    ::common::logger::logException((module), static_cast<const std::exception&>(e), __FILE__, __LINE__, ::Poco::format(fmt, ##__VA_ARGS__))

#define COMMON_LOGGER_LOG_STATUSOR_ERR(module, res, fmt, ...) \
    ::common::logger::logStatusOr((module), (res), ::Poco::format(fmt, ##__VA_ARGS__))

#define COMMON_LOGGER_LOG_ERROR_CODE(code, fmt, ...) COMMON_LOGGER_LOG_ERROR_CODE("GLOBAL", code, fmt, ##__VA_ARGS__)
#define COMMON_LOGGER_LOG_EXCEPTION(e)               COMMON_LOGGER_LOG_EXCEPTION("GLOBAL", e)
#define COMMON_LOGGER_LOG_EXCEPTION_F(e, fmt, ...)   COMMON_LOGGER_LOG_EXCEPTION_F("GLOBAL", e, fmt, ##__VA_ARGS__)
#define COMMON_LOGGER_LOG_STATUSOR(res, fmt, ...)    COMMON_LOGGER_LOG_STATUSOR_ERR("GLOBAL", res, fmt, ##__VA_ARGS__)

#endif // COMMON_LOGGER_LOGGER_ERROR_MACROS_H
