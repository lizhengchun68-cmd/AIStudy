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

/** @brief Parsed context with validation detail for API error.message (M5b). */
struct ContextParseResult {
    bool ok() const noexcept { return success_; }
    const std::string& context_id() const { return context_id_; }
    const std::vector<ArtifactMeta>& inbound_handles() const { return inbound_handles_; }
    /** @brief M7a: request session teardown after execute when true. */
    bool context_close() const { return context_close_; }
    const ErrorCodeWrapper& error() const { return error_; }
    const std::string& detail() const { return detail_; }

    static ContextParseResult success(std::string context_id,
                                      std::vector<ArtifactMeta> inbound,
                                      bool context_close = false);
    static ContextParseResult failure(const std::string& detail);

private:
    bool success_ = false;
    std::string context_id_;
    std::vector<ArtifactMeta> inbound_handles_;
    bool context_close_ = false;
    ErrorCodeWrapper error_;
    std::string detail_;

    ContextParseResult(bool success,
                       std::string context_id,
                       std::vector<ArtifactMeta> inbound,
                       ErrorCodeWrapper error,
                       std::string detail);
};

ContextParseResult parseContextObjectDetailed(const Poco::JSON::Object::Ptr& context_obj);

/** @brief "path: reason" format for validation / store API error.message (M5b). */
std::string formatValidationDetail(const std::string& path, const std::string& reason);

/** @brief Human-readable reason when context_id fails isValidContextId. */
std::string describeInvalidContextId(const std::string& context_id);

/** @brief Human-readable reason when handle_id fails isValidHandleId. */
std::string describeInvalidHandleId(const std::string& handle_id);

} // namespace scheduler
} // namespace AIstudy

#endif // AISTUDY_SCHEDULER_CONTEXT_HANDLE_RULES_H
