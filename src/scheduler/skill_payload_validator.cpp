#include "scheduler/skill_payload_validator.h"
#include "common/status/exception/error_category.h"
#include "common/status/exception/error_codes.h"

namespace AIstudy {
namespace scheduler {

StatusOr<bool> validatePayloadAgainstManifest(const Poco::JSON::Object::Ptr& payload,
                                                const SkillManifest& manifest) {
    if (!payload) {
        return StatusOr<bool>::Fail(
            ErrorCodeWrapper(static_cast<int>(ValidationError::INVALID_INPUT),
                              ErrorCategory::VALIDATION));
    }
    if (!manifest.input_schema) {
        return StatusOr<bool>::Ok(true);
    }
    if (!manifest.input_schema->has("required")
        || !manifest.input_schema->isArray("required")) {
        return StatusOr<bool>::Ok(true);
    }

    Poco::JSON::Array::Ptr required = manifest.input_schema->getArray("required");
    for (size_t i = 0; i < required->size(); ++i) {
        const std::string key = required->getElement<std::string>(static_cast<unsigned>(i));
        if (!payload->has(key)) {
            return StatusOr<bool>::Fail(
                ErrorCodeWrapper(static_cast<int>(ValidationError::INVALID_INPUT),
                                  ErrorCategory::VALIDATION));
        }
    }
    return StatusOr<bool>::Ok(true);
}

} // namespace scheduler
} // namespace AIstudy
