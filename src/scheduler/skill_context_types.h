#ifndef AISTUDY_SCHEDULER_SKILL_CONTEXT_TYPES_H
#define AISTUDY_SCHEDULER_SKILL_CONTEXT_TYPES_H

#include <string>

namespace AIstudy {
namespace scheduler {

/** @brief FEM artifact handle category (prefix mesh_ / result_ / file_). */
enum class HandleKind {
    Unknown = 0,
    Mesh,
    Result,
    File,
};

/** @brief Metadata for a session-scoped resource (no pointers cross API). */
struct ArtifactMeta {
    std::string handle_id;
    HandleKind kind = HandleKind::Unknown;
    std::string uri;
    std::string skill_id;
    std::string created_at;
};

/** @brief Parsed inbound context from envelope (M5b: filled by skill_protocol). */
struct SkillContextInbound {
    std::string context_id;
    /** handle_id -> meta from envelope context.handles */
    // Stored as vector in M5a parse helper; map in M5b store
};

const char* handleKindToString(HandleKind kind);
HandleKind handleKindFromPrefix(const std::string& handle_id);

} // namespace scheduler
} // namespace AIstudy

#endif // AISTUDY_SCHEDULER_SKILL_CONTEXT_TYPES_H
