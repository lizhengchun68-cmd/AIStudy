#include "scheduler/skill_protocol.h"
#include "common/status/exception/error_category.h"
#include "common/status/exception/error_codes.h"
#include <Poco/Dynamic/Var.h>
#include <Poco/Exception.h>
#include <Poco/JSON/Parser.h>
#include <Poco/UUIDGenerator.h>

namespace AIstudy {
namespace scheduler {

std::string ensureRequestId(const std::string& request_id) {
    if (!request_id.empty()) {
        return request_id;
    }
    return Poco::UUIDGenerator::defaultGenerator().createRandom().toString();
}

StatusOr<SkillEnvelope> parseSkillEnvelope(const std::string& envelope_json) {
    try {
        Poco::JSON::Parser parser;
        Poco::Dynamic::Var parsed = parser.parse(envelope_json);
        Poco::JSON::Object::Ptr envelope = parsed.extract<Poco::JSON::Object::Ptr>();
        if (!envelope) {
            return StatusOr<SkillEnvelope>::Fail(
                ErrorCodeWrapper(static_cast<int>(ValidationError::INVALID_INPUT),
                                  ErrorCategory::VALIDATION));
        }

        if (!envelope->has("protocol")
            || envelope->getValue<std::string>("protocol") != "1") {
            return StatusOr<SkillEnvelope>::Fail(
                ErrorCodeWrapper(static_cast<int>(ValidationError::INVALID_INPUT),
                                  ErrorCategory::VALIDATION));
        }

        SkillEnvelope out;
        if (envelope->has("request_id")) {
            out.request_id = envelope->getValue<std::string>("request_id");
        }
        out.request_id = ensureRequestId(out.request_id);

        if (envelope->has("skill_id")) {
            out.skill_id = envelope->getValue<std::string>("skill_id");
        } else {
            return StatusOr<SkillEnvelope>::Fail(
                ErrorCodeWrapper(static_cast<int>(ValidationError::INVALID_INPUT),
                                  ErrorCategory::VALIDATION));
        }

        if (envelope->has("skill_version")) {
            out.skill_version = envelope->getValue<std::string>("skill_version");
        }

        if (!envelope->has("payload")) {
            return StatusOr<SkillEnvelope>::Fail(
                ErrorCodeWrapper(static_cast<int>(ValidationError::INVALID_INPUT),
                                  ErrorCategory::VALIDATION));
        }
        out.payload = envelope->getObject("payload");
        if (!out.payload) {
            return StatusOr<SkillEnvelope>::Fail(
                ErrorCodeWrapper(static_cast<int>(ValidationError::INVALID_INPUT),
                                  ErrorCategory::VALIDATION));
        }

        return StatusOr<SkillEnvelope>::Ok(std::move(out));
    } catch (const Poco::Exception&) {
        return StatusOr<SkillEnvelope>::Fail(
            ErrorCodeWrapper(static_cast<int>(JsonError::PARSE_ERROR), ErrorCategory::JSON));
    } catch (...) {
        return StatusOr<SkillEnvelope>::Fail(
            ErrorCodeWrapper(static_cast<int>(JsonError::PARSE_ERROR), ErrorCategory::JSON));
    }
}

} // namespace scheduler
} // namespace AIstudy
