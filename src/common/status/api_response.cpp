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

namespace {

Poco::JSON::Object::Ptr makeMetaObject(const ApiResponseMeta& meta) {
    Poco::JSON::Object::Ptr m(new Poco::JSON::Object);
    if (!meta.skill_id.empty()) {
        m->set("skill_id", meta.skill_id);
    }
    if (!meta.skill_version.empty()) {
        m->set("skill_version", meta.skill_version);
    }
    m->set("duration_ms", meta.duration_ms);
    return m;
}

} // namespace

std::string makeProtocolV1SuccessResponse(const Poco::Dynamic::Var& result,
                                          const std::string& request_id,
                                          const ApiResponseMeta& meta) {
    Poco::JSON::Object::Ptr response(new Poco::JSON::Object);
    response->set("protocol", "1");
    response->set("request_id", request_id);
    response->set("ok", true);
    response->set("result", result);
    response->set("meta", makeMetaObject(meta));
    std::ostringstream oss;
    Poco::JSON::Stringifier::condense(response, oss);
    return oss.str();
}

std::string makeProtocolV1FailureResponse(const ErrorCodeWrapper& error,
                                          const std::string& request_id,
                                          const ApiResponseMeta& meta,
                                          const std::string& messageOverride) {
    Poco::JSON::Object::Ptr response(new Poco::JSON::Object);
    response->set("protocol", "1");
    response->set("request_id", request_id);
    response->set("ok", false);
    response->set("error", errorCodeToJsonObject(error, messageOverride));
    response->set("meta", makeMetaObject(meta));
    std::ostringstream oss;
    Poco::JSON::Stringifier::condense(response, oss);
    return oss.str();
}

std::string makeSkillApiResponse(bool protocol_v1,
                                 const StatusOr<Poco::JSON::Object::Ptr>& skill_result,
                                 const std::string& request_id,
                                 const ApiResponseMeta& meta) {
    if (!protocol_v1) {
        return makeApiResponse(skill_result);
    }
    if (skill_result.ok()) {
        return makeProtocolV1SuccessResponse(skill_result.value(), request_id, meta);
    }
    return makeProtocolV1FailureResponse(skill_result.status(), request_id, meta);
}

} // namespace AIstudy
