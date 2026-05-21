#ifndef AISTUDY_SCHEDULER_SKILL_MANIFEST_H
#define AISTUDY_SCHEDULER_SKILL_MANIFEST_H

#include "common/status/status_or.h"
#include <Poco/JSON/Object.h>
#include <string>
#include <vector>

namespace AIstudy {
namespace scheduler {

/** @brief Skill Manifest（与 skills/<id>/manifest.json 对应） */
struct SkillManifest {
    std::string id;
    std::string version = "1.0.0";
    std::string title;
    std::string description;
    std::vector<std::string> tags;
    Poco::JSON::Object::Ptr input_schema;
    Poco::JSON::Object::Ptr output_schema;
    int timeout_ms = 0;
    bool deprecated = false;
    std::string manifest_path;
};

StatusOr<SkillManifest> loadSkillManifest(const std::string& manifest_path);

/** @brief 将 manifest 序列化为 describe 用 JSON 对象 */
Poco::JSON::Object::Ptr skillManifestToJson(const SkillManifest& manifest);

} // namespace scheduler
} // namespace AIstudy

#endif // AISTUDY_SCHEDULER_SKILL_MANIFEST_H
