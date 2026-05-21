#ifndef AISTUDY_RAINFLOW_ADAPTER_H
#define AISTUDY_RAINFLOW_ADAPTER_H

#include "common/status/status_or.h"
#include <Poco/JSON/Object.h>
#include <string>

namespace AIstudy {
namespace adapter {
namespace rainflow {

/**
 * @brief 雨流 Skill：payload JSON �?StatusOr<result 对象>
 *
 * 仅含业务 result（items、num_cycles 等）；API 信封�?scheduler 通过 makeSkillApiResponse 生成�?
 */
StatusOr<Poco::JSON::Object::Ptr> rainflow_execute(const std::string& payload_json_str);

} // namespace rainflow
} // namespace adapter
} // namespace AIstudy

#endif // AISTUDY_RAINFLOW_ADAPTER_H
