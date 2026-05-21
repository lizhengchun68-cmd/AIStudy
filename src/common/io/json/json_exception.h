#ifndef COMMON_IO_JSON_JSON_EXCEPTION_H
#define COMMON_IO_JSON_JSON_EXCEPTION_H

#include "common/status/exception/exception.h"

namespace AIstudy {
namespace common {
namespace io {
namespace json {


/**
 * @brief JSON parse exception
 */
class JsonParseException : public SimUtilsException {
public:
    JsonParseException(const std::string& message, const ErrorCodeWrapper& errorCode,
                       const ExceptionContext& context = {})
        : SimUtilsException(message, errorCode, context) {}
};

/**
 * @brief JSON file I/O exception
 */
class JsonFileException : public SimUtilsException {
public:
    JsonFileException(const std::string& message, const ErrorCodeWrapper& errorCode,
                      const ExceptionContext& context = {})
        : SimUtilsException(message, errorCode, context) {}
};

/**
 * @brief JSON serialize exception
 */
class JsonSerializeException : public SimUtilsException {
public:
    JsonSerializeException(const std::string& message, const ErrorCodeWrapper& errorCode,
                           const ExceptionContext& context = {})
        : SimUtilsException(message, errorCode, context) {}
};

} // namespace json
} // namespace io
} // namespace common
} // namespace AIstudy


#endif // COMMON_IO_JSON_JSON_EXCEPTION_H
