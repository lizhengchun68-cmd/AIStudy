#include "scheduler/Dispatcher.h"

#include <Poco/Dynamic/Var.h>
#include <Poco/Exception.h>
#include <Poco/JSON/Object.h>
#include <Poco/JSON/Parser.h>
#include <Poco/JSON/Stringifier.h>
#include <sstream>

namespace AIstudy {
namespace scheduler {

void Dispatcher::registerAdapter(const std::string& task_type, AdapterFunc func) {
    registry_[task_type] = func;
}

std::string Dispatcher::makeErrorResponse(const std::string& msg) const {
    Poco::JSON::Object::Ptr err = new Poco::JSON::Object;
    err->set("success", false);
    err->set("error", msg);
    std::ostringstream oss;
    Poco::JSON::Stringifier::condense(err, oss);
    return oss.str();
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
            return makeErrorResponse("Unknown task_type: " + task_type);
        }

        Poco::JSON::Object::Ptr payload_obj = envelope->getObject("payload");
        std::ostringstream payload_oss;
        Poco::JSON::Stringifier::condense(payload_obj, payload_oss);
        const std::string payload_str = payload_oss.str();

        return it->second(payload_str);

    } catch (const Poco::Exception& e) {
        return makeErrorResponse(std::string("JSON parse error: ") + e.what());
    } catch (const std::exception& e) {
        return makeErrorResponse(std::string("Unexpected error: ") + e.what());
    }
}

std::vector<std::string> Dispatcher::listTaskTypes() const {
    std::vector<std::string> types;
    for (const auto& pair : registry_) {
        types.push_back(pair.first);
    }
    return types;
}

} // namespace scheduler
} // namespace AIstudy
