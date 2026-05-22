#include "scheduler/skill_protocol.h"
#include "scheduler/context_handle_rules.h"
#include "common/status/exception/error_category.h"
#include "common/status/exception/error_codes.h"
#include <Poco/Dynamic/Var.h>
#include <Poco/Exception.h>
#include <Poco/JSON/Parser.h>
#include <Poco/UUIDGenerator.h>

namespace AIstudy {
namespace scheduler {
namespace {

thread_local std::string g_last_envelope_validation_detail;

} // namespace

std::string ensureRequestId(const std::string& request_id) {
    if (!request_id.empty()) {
        return request_id;
    }
    return Poco::UUIDGenerator::defaultGenerator().createRandom().toString();
}

const std::string& lastEnvelopeValidationDetail() {
    return g_last_envelope_validation_detail;
}

StatusOr<SkillEnvelope> parseSkillEnvelope(const std::string& envelope_json) {
    g_last_envelope_validation_detail.clear();
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

        if (envelope->has("options") && envelope->isObject("options")) {
            Poco::JSON::Object::Ptr options = envelope->getObject("options");
            if (options && options->has("timeout_ms")) {
                out.timeout_ms = options->getValue<int>("timeout_ms");
            }
        }

        if (envelope->has("context")) {
            Poco::JSON::Object::Ptr context_obj;
            if (envelope->isObject("context")) {
                context_obj = envelope->getObject("context");
            }
            const auto ctx = parseContextObjectDetailed(context_obj);
            if (!ctx.ok()) {
                g_last_envelope_validation_detail = ctx.detail();
                return StatusOr<SkillEnvelope>::Fail(ctx.error());
            }
            out.context_id = ctx.context_id();
            out.inbound_handles = ctx.inbound_handles();
            out.context_close = ctx.context_close();
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
