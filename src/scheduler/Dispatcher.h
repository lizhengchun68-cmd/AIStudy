#ifndef DISPATCHER_H
#define DISPATCHER_H

#include "common/status/status_or.h"
#include "scheduler/skill_manifest.h"
#include <Poco/JSON/Object.h>
#include <string>
#include <unordered_map>
#include <vector>

namespace AIstudy {
namespace scheduler {

/** @brief 内置 Skill 注册失败记录（manifest 缺失或解析失败） */
struct SkillLoadFailure {
    std::string binding_id;
    std::string manifest_path;
    std::string error;
};

/** @brief Skill 执行结果：成功时为写�?API `result` 字段�?JSON 对象 */
using SkillResultJson = Poco::JSON::Object::Ptr;

/**
 * @brief Skill 适配器：payload JSON 字符�?�?StatusOr<result 对象>
 *
 * 不在此层生成 API 信封；由 Dispatcher::execute 统一调用 makeSkillApiResponse�?
 */
using SkillExecuteFunc = StatusOr<SkillResultJson> (*)(const std::string& payload_json);

class Dispatcher {
public:
    void registerSkill(const SkillManifest& manifest, SkillExecuteFunc func);

    /** @brief 解析调度信封，校�?payload，调�?Skill，序列化为协�?v1 JSON */
    std::string execute(const std::string& envelope_json);

    /** @brief 列出已注�?Skill 摘要 */
    std::string listSkillsJson() const;

    /** @brief 返回 manifest 描述 JSON */
    std::string describeSkillJson(const std::string& skill_id) const;

    void recordLoadFailure(SkillLoadFailure failure);
    const std::vector<SkillLoadFailure>& loadFailures() const { return load_failures_; }

private:
    struct SkillEntry {
        SkillExecuteFunc func;
        SkillManifest manifest;
    };

    std::unordered_map<std::string, SkillEntry> registry_;
    std::vector<SkillLoadFailure> load_failures_;

    const SkillEntry* findSkill(const std::string& skill_id) const;
};

} // namespace scheduler
} // namespace AIstudy

#endif // DISPATCHER_H
