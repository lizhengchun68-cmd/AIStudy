#ifndef AISTUDY_SCHEDULER_SKILL_PROTOCOL_H
#define AISTUDY_SCHEDULER_SKILL_PROTOCOL_H

#include "common/status/status_or.h"
#include <Poco/JSON/Object.h>
#include <string>

namespace AIstudy {
namespace scheduler {

/** @brief 解析后的调度信封（遗留 task_type 或 protocol v1 skill_id） */
struct SkillEnvelope {
    bool protocol_v1 = false;
    std::string request_id;
    std::string skill_id;
    std::string skill_version;
    Poco::JSON::Object::Ptr payload;
};

StatusOr<SkillEnvelope> parseSkillEnvelope(const std::string& envelope_json);

/** @brief 若 request_id 为空则生成 UUID 字符串 */
std::string ensureRequestId(const std::string& request_id);

} // namespace scheduler
} // namespace AIstudy

#endif // AISTUDY_SCHEDULER_SKILL_PROTOCOL_H
