/**
 * @file hdf5_async.h
 * @brief Async HDF5 read/write returning std::future<StatusOr<T>> (HDF5_IO §6.1)
 */

#ifndef AISTUDY_COMMON_IO_HDF5_HDF5_ASYNC_H
#define AISTUDY_COMMON_IO_HDF5_HDF5_ASYNC_H

#ifdef AISTUDY_HDF5_ENABLED

#include "common/common_status/status_or.h"
#include <future>
#include <string>
#include <vector>

namespace AIstudy {
namespace common {
namespace io {
namespace hdf5 {


/** @brief Read 1D double dataset in background. Opens file, reads, closes. */
std::future<StatusOr<std::vector<double>>> readDatasetAsync(const std::string& filePath,
                                                            const std::string& path);

/** @brief Write 1D double dataset in background. Opens file (create), writes, closes. */
std::future<StatusOr<bool>> writeDatasetAsync(const std::string& filePath,
                                              const std::string& path,
                                              const std::vector<double>& data);

} // namespace hdf5
} // namespace io
} // namespace common
} // namespace AIstudy


#endif // AISTUDY_HDF5_ENABLED

#endif // AISTUDY_COMMON_IO_HDF5_HDF5_ASYNC_H
