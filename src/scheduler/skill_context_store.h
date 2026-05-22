#ifndef AISTUDY_SCHEDULER_SKILL_CONTEXT_STORE_H
#define AISTUDY_SCHEDULER_SKILL_CONTEXT_STORE_H

/**
 * @file skill_context_store.h
 * @brief Context Store interface (M5b + M7a lifecycle).
 *
 * See dosc/context-and-handles-design.md §5.
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

    /**
     * @brief Remove in-memory session only (M7a: Host/tests).
     * @return true if a session existed before drop.
     */
    StatusOr<bool> dropContext(const std::string& context_id);

    /**
     * @brief End session: drop handles and remove artifact directory (M7a).
     * @return true if a session existed in store before close.
     */
    StatusOr<bool> closeContext(const std::string& context_id);

    bool hasContext(const std::string& context_id) const;

    /** @brief Remove sessions idle longer than configured TTL (0 = disabled). */
    int purgeExpiredContexts();

    /** @brief Default idle TTL in seconds; 0 disables expiry (M7a). */
    static void setDefaultContextTtlSeconds(int seconds);
    static int defaultContextTtlSeconds();

    static ContextStore& instance();
};

} // namespace scheduler
} // namespace AIstudy

#endif // AISTUDY_SCHEDULER_SKILL_CONTEXT_STORE_H
