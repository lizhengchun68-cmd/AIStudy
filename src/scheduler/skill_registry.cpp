#include "scheduler/skill_registry.h"
#include "adapter/rainflow_adapter.h"
#include "scheduler/skill_manifest.h"
#include <Poco/File.h>

namespace AIstudy {
namespace scheduler {

namespace {

struct SkillBinding {
    const char* id;
    SkillExecuteFunc func;
};

const SkillBinding kBindings[] = {
    {"rainflow", adapter::rainflow::rainflow_execute},
};

std::string manifestPathForSkill(const std::string& root, const std::string& skill_id) {
    return root + "/skills/" + skill_id + "/manifest.json";
}

} // namespace

std::string projectRootPath() {
#ifdef AISTUDY_PROJECT_ROOT
    return AISTUDY_PROJECT_ROOT;
#else
    return ".";
#endif
}

void registerBuiltinSkills(Dispatcher& dispatcher) {
    const std::string root = projectRootPath();
    for (const auto& binding : kBindings) {
        const std::string path = manifestPathForSkill(root, binding.id);
        auto manifestRes = loadSkillManifest(path);
        if (!manifestRes.ok()) {
            continue;
        }
        dispatcher.registerSkill(manifestRes.value(), binding.func);
    }
}

} // namespace scheduler
} // namespace AIstudy
