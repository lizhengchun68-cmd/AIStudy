#ifndef AISTUDY_SCHEDULER_SKILL_PAYLOAD_VALIDATOR_H
#define AISTUDY_SCHEDULER_SKILL_PAYLOAD_VALIDATOR_H

#include "common/status/exception/error_code_wrapper.h"
#include "scheduler/skill_manifest.h"
#include <Poco/JSON/Object.h>
#include <string>

namespace AIstudy {
namespace scheduler {

/** @brief Payload schema validation outcome with optional field-level detail for API message. */
struct PayloadValidationResult {
    bool ok() const noexcept { return success_; }
    const ErrorCodeWrapper& error() const { return error_; }
    const std::string& detail() const { return detail_; }

    static PayloadValidationResult Success();
    static PayloadValidationResult Fail(ErrorCodeWrapper error, std::string detail);

private:
    bool success_ = true;
    ErrorCodeWrapper error_;
    std::string detail_;

    PayloadValidationResult(bool success, ErrorCodeWrapper error, std::string detail);
};

/**
 * aistudy-schema-v1 subset: required, type, enum, minimum/maximum,
 * properties, items, additionalProperties, minItems, maxItems.
 * (default/description are manifest documentation only; not applied by Host.)
 */
PayloadValidationResult validatePayloadAgainstManifest(const Poco::JSON::Object::Ptr& payload,
                                                       const SkillManifest& manifest);

} // namespace scheduler
} // namespace AIstudy

#endif // AISTUDY_SCHEDULER_SKILL_PAYLOAD_VALIDATOR_H
