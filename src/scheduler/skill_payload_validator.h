#ifndef AISTUDY_SCHEDULER_SKILL_PAYLOAD_VALIDATOR_H
#define AISTUDY_SCHEDULER_SKILL_PAYLOAD_VALIDATOR_H

#include "common/status/status_or.h"
#include "scheduler/skill_manifest.h"
#include <Poco/JSON/Object.h>

namespace AIstudy {
namespace scheduler {

/** @brief 按 manifest.input_schema.required 校验 payload 顶层字段 */
StatusOr<bool> validatePayloadAgainstManifest(const Poco::JSON::Object::Ptr& payload,
                                                const SkillManifest& manifest);

} // namespace scheduler
} // namespace AIstudy

#endif // AISTUDY_SCHEDULER_SKILL_PAYLOAD_VALIDATOR_H
