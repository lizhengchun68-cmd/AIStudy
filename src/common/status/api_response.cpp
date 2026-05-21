#include "common/status/api_response.h"
#include <Poco/JSON/Stringifier.h>
#include <sstream>

namespace AIstudy {
namespace {

Poco::JSON::Object::Ptr errorCodeToJsonObject(const ErrorCodeWrapper& error,
                                              const std::string& messageOverride) {
    Poco::JSON::Object::Ptr obj(new Poco::JSON::Object);
    obj->set("code", error.code());
    obj->set("category", error.category());
    const std::string& msg = messageOverride.empty() ? error.message() : messageOverride;
    obj->set("message", msg);
    return obj;
}

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

std::string condenseResponse(Poco::JSON::Object::Ptr response) {
    std::ostringstream oss;
    Poco::JSON::Stringifier::condense(response, oss);
    return oss.str();
}
} // namespace

std::string makeSkillApiFailureResponse(const ErrorCodeWrapper& error,
                                        const std::string& request_id,
                                        const ApiResponseMeta& meta,
                                        const std::string& messageOverride) {
    Poco::JSON::Object::Ptr response(new Poco::JSON::Object);
    response->set("protocol", "1");
    response->set("request_id", request_id);
    response->set("ok", false);
    response->set("error", errorCodeToJsonObject(error, messageOverride));
    response->set("meta", makeMetaObject(meta));
    return condenseResponse(response);
}

std::string makeSkillApiResponse(const StatusOr<Poco::JSON::Object::Ptr>& skill_result,
                                 const std::string& request_id,
                                 const ApiResponseMeta& meta) {
    if (!skill_result.ok()) {
        return makeSkillApiFailureResponse(skill_result.status(), request_id, meta);
    }
    Poco::JSON::Object::Ptr response(new Poco::JSON::Object);
    response->set("protocol", "1");
    response->set("request_id", request_id);
    response->set("ok", true);
    response->set("result", skill_result.value());
    response->set("meta", makeMetaObject(meta));
    return condenseResponse(response);
}
} // namespace AIstudy
