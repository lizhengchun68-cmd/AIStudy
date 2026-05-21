#ifndef AISTUDY_COMMON_IO_JSON_JSON_HDF5_BRIDGE_H
#define AISTUDY_COMMON_IO_JSON_JSON_HDF5_BRIDGE_H

#ifdef AISTUDY_HDF5_ENABLED

#include "common/common_status/status_or.h"
#include <Poco/JSON/Object.h>
#include <string>

/**
 * @file json_hdf5_bridge.h
 * @brief Minimal JSON <-> HDF5 bridge (JSON_IO SS5, HDF5_IO SS7.3)
 */

namespace AIstudy {
namespace common {
namespace io {
namespace json {

/** @brief Write top-level JSON object to HDF5: scalars as root attrs, 1D arrays as datasets. */
StatusOr<bool> jsonToHdf5(const std::string& jsonPath, const std::string& hdf5Path);

/** @brief Read root-level attributes from HDF5 into JSON (HDF5_IO §7.3). int/double/string only. */
StatusOr<Poco::JSON::Object::Ptr> hdf5ToJson(const std::string& hdf5Path);

} // namespace json
} // namespace io
} // namespace common
} // namespace AIstudy

#endif // AISTUDY_HDF5_ENABLED

#endif // AISTUDY_COMMON_IO_JSON_JSON_HDF5_BRIDGE_H
