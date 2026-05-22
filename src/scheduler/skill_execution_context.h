#ifndef AISTUDY_SCHEDULER_SKILL_EXECUTION_CONTEXT_H
#define AISTUDY_SCHEDULER_SKILL_EXECUTION_CONTEXT_H

#include "common/status/status_or.h"
#include "scheduler/skill_context_types.h"
#include <string>

namespace AIstudy {
namespace scheduler {

/**
 * @brief Per-execute thread context for stateful Skills (M5b).
 * Set by Dispatcher before adapter; cleared after execute.
 */
namespace skill_execution_context {

void setActiveContext(const std::string& context_id);
void clearActiveContext();
bool hasActiveContext();
const std::string& activeContextId();

StatusOr<ArtifactMeta> getHandle(const std::string& handle_id);
StatusOr<bool> putHandle(const ArtifactMeta& meta, const std::string& skill_id);

} // namespace skill_execution_context
} // namespace scheduler
} // namespace AIstudy

#endif // AISTUDY_SCHEDULER_SKILL_EXECUTION_CONTEXT_H
