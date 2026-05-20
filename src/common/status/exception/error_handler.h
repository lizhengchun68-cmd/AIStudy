/**
 * @file error_handler.h
 * @brief Unified error handling (ErrorHandler, ErrorHandlingStrategy)
 */

#ifndef AISTUDY_ERROR_HANDLER_H
#define AISTUDY_ERROR_HANDLER_H

#include <string>
#include <exception>
#include <functional>
#include "error_code_wrapper.h"
#include "error_info.h"

namespace AIstudy {

class SimUtilsException;

/**
 * @brief Error handling strategy (Phase 0: no Logger; LOG_ONLY is no-op)
 */
enum class ErrorHandlingStrategy {
    THROW_EXCEPTION,   ///< Throw on handleError
    RETURN_ERROR_CODE, ///< Return (caller checks); not used for handleException
    LOG_ONLY           ///< No-op until Logger available
};

/**
 * @brief Unified error handler
 */
class ErrorHandler {
public:
    /**
     * @brief Get global handler instance
     */
    static ErrorHandler& getGlobal();

    /**
     * @brief Handle error code according to strategy
     * @param error Error code
     * @param message Optional message (used when throwing)
     * @return For RETURN_ERROR_CODE: error; else success wrapper (throws or no-op)
     */
    ErrorCodeWrapper handleError(const ErrorCodeWrapper& error,
                                 const std::string& message = "");

    /**
     * @brief Handle exception: extract ErrorInfo; according to strategy may rethrow or no-op
     * @param e Exception (typically SimUtilsException or std::exception)
     */
    void handleException(const std::exception& e);

    void setStrategy(ErrorHandlingStrategy s) noexcept { strategy_ = s; }
    ErrorHandlingStrategy strategy() const noexcept { return strategy_; }

    /**
     * @brief Optional callback (e.g. for logging); called with ErrorInfo when handling
     */
    void setCallback(std::function<void(const ErrorInfo&)> fn) { callback_ = std::move(fn); }

private:
    ErrorHandler() = default;
    ErrorHandlingStrategy strategy_ = ErrorHandlingStrategy::THROW_EXCEPTION;
    std::function<void(const ErrorInfo&)> callback_;
};

} // namespace AIstudy

#endif // AISTUDY_ERROR_HANDLER_H
