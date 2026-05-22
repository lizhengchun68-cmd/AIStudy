#include "scheduler/skill_execution_context.h"
#include "common/status/exception/error_category.h"
#include "common/status/exception/error_codes.h"
#include "scheduler/context_handle_rules.h"
#include "scheduler/context_store_errors.h"
#include "scheduler/skill_context_store.h"
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace AIstudy {
namespace scheduler {
namespace skill_execution_context {
namespace {

thread_local std::string g_active_context_id;

std::string nowIso8601Utc() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm utc{};
#ifdef _WIN32
    gmtime_s(&utc, &t);
#else
    gmtime_r(&t, &utc);
#endif
    std::ostringstream oss;
    oss << std::put_time(&utc, "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}

} // namespace

void setActiveContext(const std::string& context_id) {
    g_active_context_id = context_id;
}

void clearActiveContext() {
    g_active_context_id.clear();
}

bool hasActiveContext() {
    return !g_active_context_id.empty();
}

const std::string& activeContextId() {
    return g_active_context_id;
}

StatusOr<ArtifactMeta> getHandle(const std::string& handle_id) {
    if (!hasActiveContext()) {
        recordContextStoreFailure(
            formatValidationDetail("context", "no active context (envelope context required)"));
        return StatusOr<ArtifactMeta>::Fail(
            ErrorCodeWrapper(static_cast<int>(ValidationError::INVALID_INPUT),
                             ErrorCategory::VALIDATION));
    }
    return ContextStore::instance().getArtifact(g_active_context_id, handle_id);
}

StatusOr<bool> putHandle(const ArtifactMeta& meta, const std::string& skill_id) {
    if (!hasActiveContext()) {
        recordContextStoreFailure(
            formatValidationDetail("context", "no active context (envelope context required)"));
        return StatusOr<bool>::Fail(
            ErrorCodeWrapper(static_cast<int>(ValidationError::INVALID_INPUT),
                             ErrorCategory::VALIDATION));
    }
    ArtifactMeta stored = meta;
    stored.skill_id = skill_id;
    if (stored.created_at.empty()) {
        stored.created_at = nowIso8601Utc();
    }
    return ContextStore::instance().putArtifact(g_active_context_id, stored);
}

} // namespace skill_execution_context
} // namespace scheduler
} // namespace AIstudy
