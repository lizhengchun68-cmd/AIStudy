#ifndef AISTUDY_MESH_IMPORT_ADAPTER_H
#define AISTUDY_MESH_IMPORT_ADAPTER_H

#include "common/status/status_or.h"
#include <Poco/JSON/Object.h>
#include <string>

namespace AIstudy {
namespace adapter {
namespace mesh_import {

StatusOr<Poco::JSON::Object::Ptr> mesh_import_execute(const std::string& payload_json_str);

} // namespace mesh_import
} // namespace adapter
} // namespace AIstudy

#endif // AISTUDY_MESH_IMPORT_ADAPTER_H
