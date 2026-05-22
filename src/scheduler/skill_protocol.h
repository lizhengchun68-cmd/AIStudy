#ifndef AISTUDY_SCHEDULER_SKILL_PROTOCOL_H
#define AISTUDY_SCHEDULER_SKILL_PROTOCOL_H

#include "common/status/status_or.h"
#include <Poco/JSON/Object.h>
#include <string>

namespace AIstudy {
namespace scheduler {

/** @brief 解析后的协议 v1 调度信封 */
struct SkillEnvelope {
    std::string request_id;
    std::string skill_id;
    std::string skill_version;
    Poco::JSON::Object::Ptr payload;
    /** @brief From options.timeout_ms; 0 means unset. Cancellation not implemented (M3 logs only). */
    int timeout_ms = 0;
};

StatusOr<SkillEnvelope> parseSkillEnvelope(const std::string& envelope_json);

/** @brief �?request_id 为空则生�?UUID 字符�?*/
std::string ensureRequestId(const std::string& request_id);

} // namespace scheduler
} // namespace AIstudy

#endif // AISTUDY_SCHEDULER_SKILL_PROTOCOL_H
