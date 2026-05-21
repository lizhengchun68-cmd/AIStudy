/**
 * @file hdf5_factory.h
 * @brief H5Factory: one-liner OpenFile, WriteDataset, ReadDataset, WriteAttribute (HDF5_IO §5.1)
 */

#ifndef AISTUDY_COMMON_IO_HDF5_HDF5_FACTORY_H
#define AISTUDY_COMMON_IO_HDF5_HDF5_FACTORY_H

#ifdef AISTUDY_HDF5_ENABLED

#include "common/common_status/status_or.h"
#include "common/io/hdf5/hdf5_handles.h"
#include <map>
#include <string>
#include <vector>

namespace AIstudy {
namespace common {
namespace io {
namespace hdf5 {


/**
 * @brief Factory helpers for one-off HDF5 ops: open file, write/read dataset, write attribute.
 */
struct H5Factory {
    /** Open file read-only. */
    static StatusOr<H5FileHandle> OpenFile(const std::string& filepath);

    /** Create file (truncate) and return handle. */
    static StatusOr<H5FileHandle> CreateFile(const std::string& filepath);

    /** Open file read-write; create if not exists when \a createIfNotExists. */
    static StatusOr<H5FileHandle> OpenFileReadWrite(const std::string& filepath, bool createIfNotExists = false);

    /** Write 1D double dataset: create file if needed, write, close. */
    static StatusOr<bool> WriteDataset(const std::string& filepath, const std::string& ds_path,
                                       const std::vector<double>& data);

    /** Write 1D int dataset. */
    static StatusOr<bool> WriteDataset(const std::string& filepath, const std::string& ds_path,
                                       const std::vector<int>& data);

    /** Read 1D double dataset: open read-only, read, close. */
    static StatusOr<std::vector<double>> ReadDatasetDouble(const std::string& filepath, const std::string& ds_path);

    /** Read 1D int dataset. */
    static StatusOr<std::vector<int>> ReadDatasetInt(const std::string& filepath, const std::string& ds_path);

    /** Write scalar attribute at \a obj_path (group/dataset root). */
    static StatusOr<bool> WriteAttribute(const std::string& filepath, const std::string& obj_path,
                                         const std::string& attr_name, double value);
    static StatusOr<bool> WriteAttribute(const std::string& filepath, const std::string& obj_path,
                                         const std::string& attr_name, int value);
    static StatusOr<bool> WriteAttribute(const std::string& filepath, const std::string& obj_path,
                                         const std::string& attr_name, const std::string& value);
};

} // namespace hdf5
} // namespace io
} // namespace common
} // namespace AIstudy


#endif // AISTUDY_HDF5_ENABLED

#endif // AISTUDY_COMMON_IO_HDF5_HDF5_FACTORY_H
