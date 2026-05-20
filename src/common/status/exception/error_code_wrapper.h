/**
 * @file error_code_wrapper.h
 * @brief Main error code wrapper class based on Poco::ErrorCode
 */

#ifndef AISTUDY_ERROR_CODE_WRAPPER_H
#define AISTUDY_ERROR_CODE_WRAPPER_H

#include <string>
#include <Poco/Error.h>
#include "common/status/exception/error_category.h"

namespace AIstudy {

struct ExceptionContext;
struct ErrorInfo;
class SimUtilsException;

/**
 * @brief Error code wrapper class for industrial simulation software
 * 
 * This class provides a wrapper around Poco::ErrorCode with additional
 * functionality for simulation-specific error handling.
 */
class ErrorCodeWrapper {
public:
    /**
     * @brief Default constructor
     */
    ErrorCodeWrapper();

    /**
     * @brief Constructor with error code
     * @param code Error code value
     */
    explicit ErrorCodeWrapper(int code);

    /**
     * @brief Constructor with error code and category
     * @param code Error code value
     * @param category Error category name
     */
    ErrorCodeWrapper(int code, const std::string& category);

    /**
     * @brief Destructor
     */
    ~ErrorCodeWrapper() = default;

    // Copy and move semantics
    ErrorCodeWrapper(const ErrorCodeWrapper&) = default;
    ErrorCodeWrapper& operator=(const ErrorCodeWrapper&) = default;
    ErrorCodeWrapper(ErrorCodeWrapper&&) noexcept = default;
    ErrorCodeWrapper& operator=(ErrorCodeWrapper&&) noexcept = default;

    /**
     * @brief Get the error code value
     * @return Error code integer value
     */
    int code() const noexcept;

    /**
     * @brief Get the error category
     * @return Error category string
     */
    const std::string& category() const noexcept;

    /**
     * @brief Get the error message
     * @return Human-readable error message
     */
    std::string message() const;

    /**
     * @brief Check if error code represents success
     * @return true if code is 0 (success), false otherwise
     */
    bool isSuccess() const noexcept;

    /**
     * @brief Check if error code represents failure
     * @return true if code is non-zero (failure), false otherwise
     */
    bool isFailure() const noexcept;

    /**
     * @brief Convert to boolean (true if failure)
     */
    explicit operator bool() const noexcept;

    /**
     * @brief Comparison operators
     */
    bool operator==(const ErrorCodeWrapper& other) const noexcept;
    bool operator!=(const ErrorCodeWrapper& other) const noexcept;
    bool operator<(const ErrorCodeWrapper& other) const noexcept;

    /**
     * @brief Convert from system error code
     * @param systemErrorCode System error code (e.g., errno)
     * @return ErrorCodeWrapper instance
     */
    static ErrorCodeWrapper fromSystemError(int systemErrorCode);

    /**
     * @brief Convert to system error code
     * @return System error code, or -1 if not applicable
     */
    int toSystemError() const;

    /**
     * @brief Get formatted error string
     * @return Formatted string: "category:code - message"
     */
    std::string toString() const;

    /**
     * @brief Convert to unified ErrorInfo
     */
    ErrorInfo toErrorInfo() const;

    /** @brief Convert to system error (errno-style). Wrapper for toSystemError(). */
    static int convertToSystemError(const ErrorCodeWrapper& w) { return w.toSystemError(); }

    /** @brief Convert from system error. Wrapper for fromSystemError(). */
    static ErrorCodeWrapper convertFromSystemError(int sysErr) { return fromSystemError(sysErr); }

    /**
     * @brief Convert to exception and throw (category selects type, e.g. simulation→SimulationException)
     */
    [[noreturn]] void toException(const std::string& message = "") const;
    [[noreturn]] void toException(const std::string& message,
                                  const char* file, int line,
                                  const char* function) const;

private:
    int code_;
    std::string category_;
};

} // namespace AIstudy

#endif // AISTUDY_ERROR_CODE_WRAPPER_H
