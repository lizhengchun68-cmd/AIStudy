#include "common/status/api_response.h"
#include <Poco/JSON/Stringifier.h>
#include <sstream>

namespace AIstudy {

Poco::JSON::Object::Ptr errorCodeToJsonObject(const ErrorCodeWrapper& error,
                                              const std::string& messageOverride) {
    Poco::JSON::Object::Ptr obj(new Poco::JSON::Object);
    obj->set("code", error.code());
    obj->set("category", error.category());
    const std::string& msg = messageOverride.empty() ? error.message() : messageOverride;
    obj->set("message", msg);
    return obj;
}

void attachApiError(Poco::JSON::Object::Ptr response,
                    const ErrorCodeWrapper& error,
                    const std::string& messageOverride) {
    response->set("success", false);
    response->set("error", errorCodeToJsonObject(error, messageOverride));
}

std::string makeApiSuccessResponse(const Poco::Dynamic::Var& result) {
    Poco::JSON::Object::Ptr response(new Poco::JSON::Object);
    response->set("success", true);
    response->set("result", result);
    std::ostringstream oss;
    Poco::JSON::Stringifier::condense(response, oss);
    return oss.str();
}

std::string makeApiFailureResponse(const ErrorCodeWrapper& error,
                                   const std::string& messageOverride) {
    Poco::JSON::Object::Ptr response(new Poco::JSON::Object);
    attachApiError(response, error, messageOverride);
    std::ostringstream oss;
    Poco::JSON::Stringifier::condense(response, oss);
    return oss.str();
}

std::string makeApiFailureResponse(const ErrorInfo& info) {
    return makeApiFailureResponse(info.errorCode, info.message);
}

std::string makeApiFailureResponse(int code,
                                   const std::string& category,
                                   const std::string& message) {
    return makeApiFailureResponse(ErrorCodeWrapper(code, category), message);
}

std::string makeApiResponse(const StatusOr<Poco::JSON::Object::Ptr>& skill_result) {
    if (skill_result.ok()) {
        return makeApiSuccessResponse(skill_result.value());
    }
    return makeApiFailureResponse(skill_result.status());
}

} // namespace AIstudy
