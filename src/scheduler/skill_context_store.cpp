#include "scheduler/skill_context_store.h"
#include "common/status/exception/error_category.h"
#include "common/status/exception/error_codes.h"

namespace AIstudy {
namespace scheduler {

ContextStore& ContextStore::instance() {
    static ContextStore store;
    return store;
}

StatusOr<bool> ContextStore::ensureContext(const std::string& /*context_id*/) {
    return StatusOr<bool>::Fail(
        ErrorCodeWrapper(static_cast<int>(SystemError::UNKNOWN_ERROR), ErrorCategory::SYSTEM));
}

StatusOr<ArtifactMeta> ContextStore::getArtifact(const std::string& /*context_id*/,
                                                 const std::string& /*handle_id*/) const {
    return StatusOr<ArtifactMeta>::Fail(
        ErrorCodeWrapper(static_cast<int>(SystemError::UNKNOWN_ERROR), ErrorCategory::SYSTEM));
}

StatusOr<bool> ContextStore::putArtifact(const std::string& /*context_id*/,
                                         const ArtifactMeta& /*meta*/) {
    return StatusOr<bool>::Fail(
        ErrorCodeWrapper(static_cast<int>(SystemError::UNKNOWN_ERROR), ErrorCategory::SYSTEM));
}

StatusOr<std::vector<ArtifactMeta>> ContextStore::listArtifacts(
    const std::string& /*context_id*/) const {
    return StatusOr<std::vector<ArtifactMeta>>::Fail(
        ErrorCodeWrapper(static_cast<int>(SystemError::UNKNOWN_ERROR), ErrorCategory::SYSTEM));
}

StatusOr<bool> ContextStore::mergeInbound(const std::string& /*context_id*/,
                                          const std::vector<ArtifactMeta>& /*inbound*/) {
    return StatusOr<bool>::Fail(
        ErrorCodeWrapper(static_cast<int>(SystemError::UNKNOWN_ERROR), ErrorCategory::SYSTEM));
}

} // namespace scheduler
} // namespace AIstudy
