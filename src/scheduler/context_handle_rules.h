#ifndef AISTUDY_SCHEDULER_CONTEXT_HANDLE_RULES_H
#define AISTUDY_SCHEDULER_CONTEXT_HANDLE_RULES_H

#include "common/status/status_or.h"
#include "scheduler/skill_context_types.h"
#include <Poco/JSON/Object.h>
#include <string>
#include <vector>

namespace AIstudy {
namespace scheduler {

/** @brief Validate context_id string (M5a rules). */
bool isValidContextId(const std::string& context_id);

/** @brief Validate handle_id prefix and charset. */
bool isValidHandleId(const std::string& handle_id);

/** @brief If kind is not Unknown, must match handle_id prefix. */
bool handleKindMatchesId(const std::string& handle_id, HandleKind kind);

/** @brief Parse ArtifactMeta from one handle object; fails with detail path. */
StatusOr<ArtifactMeta> parseHandleEntry(const std::string& handle_id,
                                         const Poco::JSON::Object::Ptr& obj,
                                         const std::string& path_prefix);

/**
 * @brief Parse envelope "context" object (optional). Empty object -> empty context_id.
 * Fails if context key present but invalid.
 */
StatusOr<std::pair<std::string, std::vector<ArtifactMeta>>> parseContextObject(
    const Poco::JSON::Object::Ptr& context_obj);

} // namespace scheduler
} // namespace AIstudy

#endif // AISTUDY_SCHEDULER_CONTEXT_HANDLE_RULES_H
