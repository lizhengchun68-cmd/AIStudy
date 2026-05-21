#include "common/common_status/exception/error_handler.h"
#include "common/common_status/exception/exception.h"

namespace AIstudy {

ErrorHandler& ErrorHandler::getGlobal() {
    static ErrorHandler instance;
    return instance;
}

ErrorCodeWrapper ErrorHandler::handleError(const ErrorCodeWrapper& error,
                                           const std::string& message) {
    if (error.isSuccess()) return error;
    ErrorInfo info = getErrorInfo(error);
    if (callback_) callback_(info);
    switch (strategy_) {
    case ErrorHandlingStrategy::THROW_EXCEPTION:
        error.toException(message.empty() ? info.message : message);
        /* never reached */
    case ErrorHandlingStrategy::LOG_ONLY:
        break;
    case ErrorHandlingStrategy::RETURN_ERROR_CODE:
        break;
    }
    return error;
}

void ErrorHandler::handleException(const std::exception& e) {
    ErrorInfo info = extractErrorInfo(e);
    if (callback_) callback_(info);
    switch (strategy_) {
    case ErrorHandlingStrategy::THROW_EXCEPTION: {
        ErrorCodeWrapper ec = extractErrorCode(e);
        ec.toException(info.message);
    }
    case ErrorHandlingStrategy::LOG_ONLY:
        break;
    case ErrorHandlingStrategy::RETURN_ERROR_CODE:
        break;
    }
}

} // namespace AIstudy
