#include "scheduler/skill_registry.h"
#include "adapter/rainflow_adapter.h"
#include "common/logger/logger.h"
#include "scheduler/skill_manifest.h"
#include <Poco/File.h>
#include <sstream>

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

std::string manifestErrorMessage(const StatusOr<SkillManifest>& manifestRes) {
    std::ostringstream oss;
    oss << "code=" << manifestRes.status().code()
        << " category=" << manifestRes.status().category();
    return oss.str();
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
    registerBuiltinSkills(dispatcher, projectRootPath());
}

void registerBuiltinSkills(Dispatcher& dispatcher, const std::string& project_root) {
    for (const auto& binding : kBindings) {
        const std::string path = manifestPathForSkill(project_root, binding.id);
        auto manifestRes = loadSkillManifest(path);
        if (!manifestRes.ok()) {
            SkillLoadFailure failure;
            failure.binding_id = binding.id;
            failure.manifest_path = path;
            failure.error = manifestErrorMessage(manifestRes);
            dispatcher.recordLoadFailure(std::move(failure));
            common::logger::Logger::get("AIstudy.SkillRegistry").warning(
                std::string("Skill manifest load failed: id=") + binding.id + " path=" + path
                + " " + failure.error);
            continue;
        }
        dispatcher.registerSkill(manifestRes.value(), binding.func);
        common::logger::Logger::get("AIstudy.SkillRegistry").info(
            std::string("Skill registered: id=") + binding.id + " path=" + path);
    }
}

} // namespace scheduler
} // namespace AIstudy
