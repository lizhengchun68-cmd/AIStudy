#ifndef AISTUDY_SCHEDULER_SKILL_PROTOCOL_H
#define AISTUDY_SCHEDULER_SKILL_PROTOCOL_H

#include "common/status/status_or.h"
#include "scheduler/skill_context_types.h"
#include <Poco/JSON/Object.h>
#include <string>
#include <vector>

namespace AIstudy {
namespace scheduler {

/** @brief 解析后的协议 v1 调度信封 */
struct SkillEnvelope {
    std::string request_id;
    std::string skill_id;
    std::string skill_version;
    Poco::JSON::Object::Ptr payload;
    int timeout_ms = 0;
    /** @brief Empty if stateless execute. */
    std::string context_id;
    std::vector<ArtifactMeta> inbound_handles;
};

StatusOr<SkillEnvelope> parseSkillEnvelope(const std::string& envelope_json);

/** @brief After failed parseSkillEnvelope, validation detail for messageOverride (M5b). */
const std::string& lastEnvelopeValidationDetail();

std::string ensureRequestId(const std::string& request_id);

} // namespace scheduler
} // namespace AIstudy

#endif // AISTUDY_SCHEDULER_SKILL_PROTOCOL_H
