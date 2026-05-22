#include "host_echo_adapter.h"
#include "common/status/exception/error_category.h"
#include "common/status/exception/error_codes.h"
#include <Poco/JSON/Object.h>
#include <Poco/JSON/Parser.h>

namespace AIstudy {
namespace adapter {
namespace host_echo {

StatusOr<Poco::JSON::Object::Ptr> host_echo_execute(const std::string& payload_json_str) {
    try {
        Poco::JSON::Parser parser;
        Poco::JSON::Object::Ptr payload =
            parser.parse(payload_json_str).extract<Poco::JSON::Object::Ptr>();
        if (!payload || !payload->has("message") || !payload->get("message").isString()) {
            return StatusOr<Poco::JSON::Object::Ptr>::Fail(
                ErrorCodeWrapper(static_cast<int>(ValidationError::INVALID_INPUT),
                                 ErrorCategory::VALIDATION));
        }
        const std::string message = payload->getValue<std::string>("message");
        Poco::JSON::Object::Ptr result(new Poco::JSON::Object);
        result->set("message", message);
        result->set("length", static_cast<int>(message.size()));
        return StatusOr<Poco::JSON::Object::Ptr>::Ok(result);
    } catch (...) {
        return StatusOr<Poco::JSON::Object::Ptr>::Fail(
            ErrorCodeWrapper(static_cast<int>(JsonError::PARSE_ERROR), ErrorCategory::JSON));
    }
}

} // namespace host_echo
} // namespace adapter
} // namespace AIstudy
