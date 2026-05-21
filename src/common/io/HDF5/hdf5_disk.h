/**
 * @file hdf5_disk.h
 * @brief Disk space checks for HDF5 write safety (HDF5_IO §6.3)
 */

#ifndef AISTUDY_COMMON_IO_HDF5_HDF5_DISK_H
#define AISTUDY_COMMON_IO_HDF5_HDF5_DISK_H

#ifdef AISTUDY_HDF5_ENABLED

#include "common/common_status/status_or.h"
#include <cstdint>
#include <string>

namespace AIstudy {
namespace common {
namespace io {
namespace hdf5 {


/** @brief Free bytes on the filesystem containing \a path. */
StatusOr<uint64_t> getDiskFreeBytes(const std::string& path);

/** @brief True if free space >= \a requiredBytes; otherwise Fail(DISK_FULL). */
StatusOr<bool> checkDiskSpace(const std::string& path, uint64_t requiredBytes);

} // namespace hdf5
} // namespace io
} // namespace common
} // namespace AIstudy


#endif // AISTUDY_HDF5_ENABLED

#endif // AISTUDY_COMMON_IO_HDF5_HDF5_DISK_H
