#ifndef AISTUDY_SCHEDULER_SKILL_CONTEXT_STORE_H
#define AISTUDY_SCHEDULER_SKILL_CONTEXT_STORE_H

/**
 * @file skill_context_store.h
 * @brief Context Store interface (M5b implementation).
 *
 * M5a: interface only — see dosc/context-and-handles-design.md §5.
 */

#include "common/status/status_or.h"
#include "scheduler/skill_context_types.h"
#include <string>
#include <vector>

namespace AIstudy {
namespace scheduler {

/** @brief In-process session store: context_id -> handle_id -> ArtifactMeta (M5b). */
class ContextStore {
public:
    StatusOr<bool> ensureContext(const std::string& context_id);

    StatusOr<ArtifactMeta> getArtifact(const std::string& context_id,
                                       const std::string& handle_id) const;

    StatusOr<bool> putArtifact(const std::string& context_id, const ArtifactMeta& meta);

    StatusOr<std::vector<ArtifactMeta>> listArtifacts(const std::string& context_id) const;

    /** @brief Merge inbound handles from envelope; overwrites same handle_id. */
    StatusOr<bool> mergeInbound(const std::string& context_id,
                                const std::vector<ArtifactMeta>& inbound);

    static ContextStore& instance();
};

} // namespace scheduler
} // namespace AIstudy

#endif // AISTUDY_SCHEDULER_SKILL_CONTEXT_STORE_H
