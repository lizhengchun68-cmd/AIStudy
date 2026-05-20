/**
 * @file error_info.h
 * @brief Unified error information (ErrorInfo), ExceptionContext, extract helpers
 */

#ifndef AISTUDY_ERROR_INFO_H
#define AISTUDY_ERROR_INFO_H

#include <string>
#include <memory>
#include <exception>
#include "error_code_wrapper.h"

namespace AIstudy {

class SimUtilsException;

/**
 * @brief Exception context (file, line, function)
 */
struct ExceptionContext {
    std::string file;
    int         line = 0;
    std::string function;

    bool empty() const noexcept {
        return file.empty() && line == 0 && function.empty();
    }
};

/**
 * @brief Format context as " at file:line in function" or empty string
 */
std::string getFormattedMessage(const ExceptionContext& ctx);

/**
 * @brief Unified error information
 */
struct ErrorInfo {
    ErrorCodeWrapper  errorCode;
    std::string       message;
    std::string       category;
    ExceptionContext  context;
    std::string       timestamp;

    std::string toString() const;
    std::string toFormattedString() const;

    /** @brief Serialize to JSON string. Fields: code, category, message, context {file,line,function}, timestamp. */
    std::string toJson() const;

    /** @brief Deserialize from JSON string. Returns default ErrorInfo on parse error. */
    static ErrorInfo fromJson(const std::string& json);
};

/**
 * @brief Get ErrorInfo from ErrorCodeWrapper
 */
ErrorInfo getErrorInfo(const ErrorCodeWrapper& error);
ErrorInfo getErrorInfo(const ErrorCodeWrapper& error, const ExceptionContext& ctx);
/** @brief Overload with explicit message (e.g. exception::what()) */
ErrorInfo getErrorInfo(const ErrorCodeWrapper& error, const ExceptionContext& ctx,
                      const std::string& messageOverride);

/**
 * @brief Extract error code from exception
 */
ErrorCodeWrapper extractErrorCode(const SimUtilsException& e);
ErrorCodeWrapper extractErrorCode(const std::exception& e);
ErrorCodeWrapper extractErrorCode(const std::exception_ptr& eptr);

/**
 * @brief Extract ErrorInfo from exception
 */
ErrorInfo extractErrorInfo(const SimUtilsException& e);
ErrorInfo extractErrorInfo(const std::exception& e);

} // namespace AIstudy

#endif // AISTUDY_ERROR_INFO_H
