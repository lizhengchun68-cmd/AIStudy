#ifndef AISTUDY_RAINFLOW_ADAPTER_H
#define AISTUDY_RAINFLOW_ADAPTER_H

#include <string>

namespace AIstudy {
namespace adapter {
namespace rainflow {
    std::string rainflow_adapter(const std::string& payload_json_str);
} // namespace rainflow
} // namespace adapter
} // namespace AIstudy

#endif // AISTUDY_RAINFLOW_ADAPTER_H