#ifndef AISTUDY_HOST_ECHO_ADAPTER_H
#define AISTUDY_HOST_ECHO_ADAPTER_H

#include "common/status/status_or.h"
#include <Poco/JSON/Object.h>
#include <string>

namespace AIstudy {
namespace adapter {
namespace host_echo {

/** @brief Minimal host Skill: echoes message (routing / multi-skill smoke test). */
StatusOr<Poco::JSON::Object::Ptr> host_echo_execute(const std::string& payload_json_str);

} // namespace host_echo
} // namespace adapter
} // namespace AIstudy

#endif // AISTUDY_HOST_ECHO_ADAPTER_H
