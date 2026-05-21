#ifndef COMMON_LOGGER_LOGGER_ERROR_H
#define COMMON_LOGGER_LOGGER_ERROR_H

#include "logger.h"
#include "common/status/exception/error_code_wrapper.h"
#include "common/status/exception/error_info.h"
#include "common/status/status_or.h"
#include "common/status/exception/exception.h"
#include <exception>
#include <string>

namespace AIstudy {
namespace common {
namespace logger {

/**
 * @brief Log ErrorCodeWrapper with unified format (code, category, message)
 */
void logError(const ErrorCodeWrapper& error, const std::string& message = "");

/**
 * @brief Log SimUtilsException (extracts ErrorInfo, unified format)
 */
void logError(const SimUtilsException& e);

/**
 * @brief Log std::exception (best-effort extract or plain what())
 */
void logError(const std::exception& e);

/**
 * @brief Log error code at given level
 */
void logErrorCode(LogLevel level, const ErrorCodeWrapper& error, const std::string& message = "");
/** @brief Default ERROR level */
void logErrorCode(const ErrorCodeWrapper& error, const std::string& message = "");

/** @brief Log error code with module (uses Logger::get(module)). Optional file/line for trace. */
void logErrorCode(const std::string& module, const ErrorCodeWrapper& error, const std::string& message = "");
void logErrorCode(LogLevel level, const std::string& module, const ErrorCodeWrapper& error, const std::string& message = "");
void logErrorCode(const std::string& module, const ErrorCodeWrapper& error, const std::string& message, const char* file, int line);
void logErrorCode(LogLevel level, const std::string& module, const ErrorCodeWrapper& error, const std::string& message, const char* file, int line);

/**
 * @brief Log exception
 */
void logException(const SimUtilsException& e);
void logException(const SimUtilsException& e, const char* file, int line);
void logException(const std::exception& e);
void logException(const std::exception& e, const char* file, int line);

/** @brief Log exception with module. Optional message appended. */
void logException(const std::string& module, const SimUtilsException& e, const char* file, int line, const std::string& message = "");
void logException(const std::string& module, const std::exception& e, const char* file, int line, const std::string& message = "");

/**
 * @brief If StatusOr failed, log ERROR with status and optional message (LOGGER §3.2).
 */
template <typename T>
void logStatusOr(const StatusOr<T>& res, const std::string& message = "") {
    if (!res.ok())
        logErrorCode(res.status(), message);
}

/** @brief StatusOr log with module. */
template <typename T>
void logStatusOr(const std::string& module, const StatusOr<T>& res, const std::string& message = "") {
    if (!res.ok())
        logErrorCode(module, res.status(), message);
}

} // namespace logger
} // namespace common
} // namespace AIstudy

#endif // COMMON_LOGGER_LOGGER_ERROR_H
