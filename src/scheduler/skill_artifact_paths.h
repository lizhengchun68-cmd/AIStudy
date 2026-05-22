#ifndef AISTUDY_SCHEDULER_SKILL_ARTIFACT_PATHS_H
#define AISTUDY_SCHEDULER_SKILL_ARTIFACT_PATHS_H

#include <string>

namespace AIstudy {
namespace scheduler {

/** @brief ${project_root}/.aistudy/artifacts */
std::string artifactsRootDir();

/** @brief ${artifactsRoot}/<context_id>/ */
std::string artifactContextDir(const std::string& context_id);

/** @brief artifact://<context_id>/<relative_path> */
std::string makeArtifactUri(const std::string& context_id, const std::string& relative_path);

/** @brief Resolve artifact URI to filesystem path under project root. */
std::string artifactUriToFilesystemPath(const std::string& uri);

/** @brief Create context artifact directory if missing. */
bool ensureArtifactContextDir(const std::string& context_id);

/** @brief Remove context artifact directory if present (M7a close/TTL). */
bool removeArtifactContextDir(const std::string& context_id);

} // namespace scheduler
} // namespace AIstudy

#endif // AISTUDY_SCHEDULER_SKILL_ARTIFACT_PATHS_H
