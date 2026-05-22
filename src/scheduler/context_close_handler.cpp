#include "scheduler/context_close_handler.h"

#include "common/status/exception/error_category.h"
#include "common/status/exception/error_codes.h"
#include "scheduler/context_handle_rules.h"
#include "scheduler/context_store_errors.h"
#include "scheduler/skill_context_store.h"
#include "scheduler/skill_execution_context.h"
#include <Poco/JSON/Object.h>

namespace AIstudy {
namespace scheduler {

StatusOr<SkillResultJson> context_close_execute(const std::string& /*payload_json*/) {
    if (!skill_execution_context::hasActiveContext()) {
        recordContextStoreFailure(formatValidationDetail(
            "context", "no active context (envelope context required)"));
        return StatusOr<SkillResultJson>::Fail(
            ErrorCodeWrapper(static_cast<int>(ValidationError::INVALID_INPUT),
                             ErrorCategory::VALIDATION));
    }

    const std::string context_id = skill_execution_context::activeContextId();
    const auto closed = ContextStore::instance().closeContext(context_id);
    skill_execution_context::clearActiveContext();

    if (!closed.ok()) {
        return StatusOr<SkillResultJson>::Fail(closed.status());
    }

    Poco::JSON::Object::Ptr result(new Poco::JSON::Object);
    result->set("closed", true);
    result->set("context_id", context_id);
    result->set("had_session", closed.value());
    return StatusOr<SkillResultJson>::Ok(result);
}

} // namespace scheduler
} // namespace AIstudy
