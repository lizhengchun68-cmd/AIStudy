#include "base/log/logger_error.h"
#include "base/exception/exception.h"

namespace AIstudy {
namespace common {
namespace logger {

namespace {

void logErrorInfo(const ErrorInfo& info, LogLevel level) {
    Logger log = Logger::defaultLogger();
    std::string s = info.toFormattedString();
    switch (level) {
        case LogLevel::TRACE:   log.trace(s);   break;
        case LogLevel::DEBUG:   log.debug(s);   break;
        case LogLevel::INFO:    log.info(s);    break;
        case LogLevel::WARNING: log.warning(s); break;
        case LogLevel::ERROR:   log.error(s);   break;
        case LogLevel::FATAL:   log.fatal(s);   break;
    }
}

void logErrorInfoTo(const std::string& module, const ErrorInfo& info, LogLevel level, const char* file, int line) {
    Logger log = Logger::get(module);
    std::string s = info.toFormattedString();
    if (file && line > 0) {
        switch (level) {
            case LogLevel::TRACE:   log.trace(s, file, line);   break;
            case LogLevel::DEBUG:   log.debug(s, file, line);   break;
            case LogLevel::INFO:    log.info(s, file, line);    break;
            case LogLevel::WARNING: log.warning(s, file, line); break;
            case LogLevel::ERROR:   log.error(s, file, line);   break;
            case LogLevel::FATAL:   log.fatal(s, file, line);   break;
        }
    } else {
        switch (level) {
            case LogLevel::TRACE:   log.trace(s);   break;
            case LogLevel::DEBUG:   log.debug(s);   break;
            case LogLevel::INFO:    log.info(s);    break;
            case LogLevel::WARNING: log.warning(s); break;
            case LogLevel::ERROR:   log.error(s);   break;
            case LogLevel::FATAL:   log.fatal(s);   break;
        }
    }
}

} // namespace

void logError(const ErrorCodeWrapper& error, const std::string& message) {
    ErrorInfo info = getErrorInfo(error);
    if (!message.empty()) info.message = message;
    logErrorInfo(info, LogLevel::ERROR);
}

void logError(const SimUtilsException& e) {
    logErrorInfo(e.toErrorInfo(), LogLevel::ERROR);
}

void logError(const std::exception& e) {
    ErrorInfo info = extractErrorInfo(e);
    logErrorInfo(info, LogLevel::ERROR);
}

void logErrorCode(LogLevel level, const ErrorCodeWrapper& error, const std::string& message) {
    ErrorInfo info = getErrorInfo(error);
    if (!message.empty()) info.message = message;
    logErrorInfo(info, level);
}

void logErrorCode(const ErrorCodeWrapper& error, const std::string& message) {
    logErrorCode(LogLevel::ERROR, error, message);
}

void logException(const SimUtilsException& e) {
    logError(e);
}

void logException(const SimUtilsException& e, const char* file, int line) {
    Logger::defaultLogger().error(e.toErrorInfo().toFormattedString(), file, line);
}

void logException(const std::exception& e) {
    logError(e);
}

void logException(const std::exception& e, const char* file, int line) {
    Logger::defaultLogger().error(extractErrorInfo(e).toFormattedString(), file, line);
}

void logErrorCode(const std::string& module, const ErrorCodeWrapper& error, const std::string& message) {
    logErrorCode(LogLevel::ERROR, module, error, message, nullptr, 0);
}

void logErrorCode(LogLevel level, const std::string& module, const ErrorCodeWrapper& error, const std::string& message) {
    logErrorCode(level, module, error, message, nullptr, 0);
}

void logErrorCode(const std::string& module, const ErrorCodeWrapper& error, const std::string& message, const char* file, int line) {
    logErrorCode(LogLevel::ERROR, module, error, message, file, line);
}

void logErrorCode(LogLevel level, const std::string& module, const ErrorCodeWrapper& error, const std::string& message, const char* file, int line) {
    ErrorInfo info = getErrorInfo(error);
    if (!message.empty()) info.message = message;
    logErrorInfoTo(module, info, level, file, line);
}

void logException(const std::string& module, const SimUtilsException& e, const char* file, int line, const std::string& message) {
    ErrorInfo info = e.toErrorInfo();
    if (!message.empty()) info.message = info.message.empty() ? message : (info.message + " - " + message);
    logErrorInfoTo(module, info, LogLevel::ERROR, file, line);
}

void logException(const std::string& module, const std::exception& e, const char* file, int line, const std::string& message) {
    ErrorInfo info = extractErrorInfo(e);
    if (!message.empty()) info.message = info.message.empty() ? message : (info.message + " - " + message);
    logErrorInfoTo(module, info, LogLevel::ERROR, file, line);
}

} // namespace logger
} // namespace common
} // namespace AIstudy
