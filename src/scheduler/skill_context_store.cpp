#include "scheduler/skill_context_store.h"
#include "common/status/exception/error_category.h"
#include "common/status/exception/error_codes.h"
#include "scheduler/context_handle_rules.h"
#include "scheduler/context_store_errors.h"
#include "scheduler/skill_artifact_paths.h"
#include <mutex>
#include <unordered_map>

namespace AIstudy {
namespace scheduler {
namespace {

using ContextMap = std::unordered_map<std::string, ArtifactMeta>;

std::mutex& storeMutex() {
    static std::mutex m;
    return m;
}

std::unordered_map<std::string, ContextMap>& contextData() {
    static std::unordered_map<std::string, ContextMap> data;
    return data;
}

ErrorCodeWrapper validationFail() {
    return ErrorCodeWrapper(static_cast<int>(ValidationError::INVALID_INPUT),
                            ErrorCategory::VALIDATION);
}

ErrorCodeWrapper systemFail() {
    return ErrorCodeWrapper(static_cast<int>(SystemError::UNKNOWN_ERROR), ErrorCategory::SYSTEM);
}

StatusOr<bool> failValidation(const std::string& detail) {
    recordContextStoreFailure(detail);
    return StatusOr<bool>::Fail(validationFail());
}

StatusOr<ArtifactMeta> failValidationMeta(const std::string& detail) {
    recordContextStoreFailure(detail);
    return StatusOr<ArtifactMeta>::Fail(validationFail());
}

std::string handleNotFoundDetail(const std::string& context_id, const std::string& handle_id) {
    return formatValidationDetail(
        "context.handles." + handle_id,
        "not registered (context_id=" + context_id + ")");
}

std::string artifactDirDetail(const std::string& context_id) {
    return formatValidationDetail(
        "context",
        "failed to create artifact directory (context_id=" + context_id + ")");
}

} // namespace

ContextStore& ContextStore::instance() {
    static ContextStore store;
    return store;
}

StatusOr<bool> ContextStore::ensureContext(const std::string& context_id) {
    clearLastContextStoreDetail();
    if (!isValidContextId(context_id)) {
        return failValidation(describeInvalidContextId(context_id));
    }
    if (!ensureArtifactContextDir(context_id)) {
        return failValidation(artifactDirDetail(context_id));
    }
    std::lock_guard<std::mutex> lock(storeMutex());
    contextData()[context_id];
    return StatusOr<bool>::Ok(true);
}

StatusOr<ArtifactMeta> ContextStore::getArtifact(const std::string& context_id,
                                                 const std::string& handle_id) const {
    clearLastContextStoreDetail();
    if (!isValidContextId(context_id)) {
        return failValidationMeta(describeInvalidContextId(context_id));
    }
    if (!isValidHandleId(handle_id)) {
        return failValidationMeta(describeInvalidHandleId(handle_id));
    }
    std::lock_guard<std::mutex> lock(storeMutex());
    const auto ctx_it = contextData().find(context_id);
    if (ctx_it == contextData().end()) {
        return failValidationMeta(handleNotFoundDetail(context_id, handle_id));
    }
    const auto handle_it = ctx_it->second.find(handle_id);
    if (handle_it == ctx_it->second.end()) {
        return failValidationMeta(handleNotFoundDetail(context_id, handle_id));
    }
    return StatusOr<ArtifactMeta>::Ok(handle_it->second);
}

StatusOr<bool> ContextStore::putArtifact(const std::string& context_id, const ArtifactMeta& meta) {
    clearLastContextStoreDetail();
    if (!isValidContextId(context_id)) {
        return failValidation(describeInvalidContextId(context_id));
    }
    if (!isValidHandleId(meta.handle_id)) {
        return failValidation(describeInvalidHandleId(meta.handle_id));
    }
    auto ensured = ensureContext(context_id);
    if (!ensured.ok()) {
        return ensured;
    }
    std::lock_guard<std::mutex> lock(storeMutex());
    contextData()[context_id][meta.handle_id] = meta;
    return StatusOr<bool>::Ok(true);
}

StatusOr<std::vector<ArtifactMeta>> ContextStore::listArtifacts(
    const std::string& context_id) const {
    clearLastContextStoreDetail();
    if (!isValidContextId(context_id)) {
        recordContextStoreFailure(describeInvalidContextId(context_id));
        return StatusOr<std::vector<ArtifactMeta>>::Fail(validationFail());
    }
    std::lock_guard<std::mutex> lock(storeMutex());
    const auto ctx_it = contextData().find(context_id);
    if (ctx_it == contextData().end()) {
        return StatusOr<std::vector<ArtifactMeta>>::Ok(std::vector<ArtifactMeta>{});
    }
    std::vector<ArtifactMeta> out;
    out.reserve(ctx_it->second.size());
    for (const auto& pair : ctx_it->second) {
        out.push_back(pair.second);
    }
    return StatusOr<std::vector<ArtifactMeta>>::Ok(std::move(out));
}

StatusOr<bool> ContextStore::dropContext(const std::string& context_id) {
    clearLastContextStoreDetail();
    std::lock_guard<std::mutex> lock(storeMutex());
    contextData().erase(context_id);
    return StatusOr<bool>::Ok(true);
}

StatusOr<bool> ContextStore::mergeInbound(const std::string& context_id,
                                          const std::vector<ArtifactMeta>& inbound) {
    clearLastContextStoreDetail();
    auto ensured = ensureContext(context_id);
    if (!ensured.ok()) {
        return ensured;
    }
    for (const auto& meta : inbound) {
        auto put = putArtifact(context_id, meta);
        if (!put.ok()) {
            return put;
        }
    }
    return StatusOr<bool>::Ok(true);
}

} // namespace scheduler
} // namespace AIstudy
