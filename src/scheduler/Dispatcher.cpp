#include "scheduler/Dispatcher.h"

#include "common/status/api_response.h"
#include "common/status/exception/error_category.h"
#include "common/status/exception/error_codes.h"
#include <Poco/Dynamic/Var.h>
#include <Poco/Exception.h>
#include <Poco/JSON/Object.h>
#include <Poco/JSON/Parser.h>
#include <Poco/JSON/Stringifier.h>
#include <sstream>

namespace AIstudy {
namespace scheduler {

void Dispatcher::registerAdapter(const std::string& task_type, SkillExecuteFunc func, const std::string& schema) {
    registry_[task_type] = {func, schema};
}

std::string Dispatcher::makeErrorResponse(const std::string& msg) const {
    return makeApiFailureResponse(
        static_cast<int>(ValidationError::INVALID_INPUT),
        ErrorCategory::VALIDATION,
        msg);
}

std::string Dispatcher::execute(const std::string& envelope_json) {
    Poco::JSON::Parser parser;
    try {
        Poco::Dynamic::Var parsed = parser.parse(envelope_json);
        Poco::JSON::Object::Ptr envelope = parsed.extract<Poco::JSON::Object::Ptr>();

        if (!envelope->has("task_type")) {
            return makeErrorResponse("Missing 'task_type' in envelope");
        }
        const std::string task_type = envelope->getValue<std::string>("task_type");

        if (!envelope->has("payload")) {
            return makeErrorResponse("Missing 'payload' in envelope");
        }

        const auto it = registry_.find(task_type);
        if (it == registry_.end()) {
            return makeApiFailureResponse(
                static_cast<int>(SystemError::UNKNOWN_ERROR),
                ErrorCategory::SYSTEM,
                "Unknown task_type: " + task_type);
        }

        Poco::JSON::Object::Ptr payload_obj = envelope->getObject("payload");
        std::ostringstream payload_oss;
        Poco::JSON::Stringifier::condense(payload_obj, payload_oss);
        const std::string payload_str = payload_oss.str();

        const StatusOr<SkillResultJson> skill_result = it->second.func(payload_str);
        return makeApiResponse(skill_result);

    } catch (const Poco::Exception& e) {
        return makeApiFailureResponse(
            static_cast<int>(JsonError::PARSE_ERROR),
            ErrorCategory::JSON,
            std::string("JSON parse error: ") + e.what());
    } catch (const std::exception& e) {
        return makeApiFailureResponse(
            static_cast<int>(SystemError::UNKNOWN_ERROR),
            ErrorCategory::SYSTEM,
            std::string("Unexpected error: ") + e.what());
    }
}

std::vector<std::string> Dispatcher::listTaskTypes() const {
    std::vector<std::string> types;
    for (const auto& pair : registry_) types.push_back(pair.first);
    return types;
}

std::string Dispatcher::getSchema(const std::string& task_type) const {
    auto it = registry_.find(task_type);
    if (it == registry_.end())
        return makeErrorResponse("Unknown task_type: " + task_type);
    return it->second.schema.empty() ? "{}" : it->second.schema;
}

} // namespace scheduler
} // namespace AIstudy
