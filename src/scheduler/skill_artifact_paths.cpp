#include "scheduler/skill_artifact_paths.h"
#include "scheduler/skill_registry.h"
#include <Poco/File.h>

namespace AIstudy {
namespace scheduler {

std::string artifactsRootDir() {
    return projectRootPath() + "/.aistudy/artifacts";
}

std::string artifactContextDir(const std::string& context_id) {
    return artifactsRootDir() + "/" + context_id;
}

std::string makeArtifactUri(const std::string& context_id, const std::string& relative_path) {
    return "artifact://" + context_id + "/" + relative_path;
}

std::string artifactUriToFilesystemPath(const std::string& uri) {
    const std::string prefix = "artifact://";
    if (uri.rfind(prefix, 0) != 0) {
        return {};
    }
    const std::string rest = uri.substr(prefix.size());
    const auto slash = rest.find('/');
    if (slash == std::string::npos) {
        return {};
    }
    const std::string context_id = rest.substr(0, slash);
    const std::string rel = rest.substr(slash + 1);
    return artifactContextDir(context_id) + "/" + rel;
}

bool ensureArtifactContextDir(const std::string& context_id) {
    try {
        Poco::File dir(artifactContextDir(context_id));
        if (!dir.exists()) {
            dir.createDirectories();
        }
        return dir.exists() && dir.isDirectory();
    } catch (...) {
        return false;
    }
}

} // namespace scheduler
} // namespace AIstudy
