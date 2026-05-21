/**
 * @file hdf5_writer.h
 * @brief HDF5 writer: createFile/openFile/closeFile, writeDataset, writeAttribute, createGroup, flush/sync
 *
 * Requires AISTUDY_HDF5_ENABLED and HDF5 C++ library.
 */

#ifndef AISTUDY_COMMON_IO_HDF5_HDF5_WRITER_H
#define AISTUDY_COMMON_IO_HDF5_HDF5_WRITER_H

#ifdef AISTUDY_HDF5_ENABLED

#include <functional>
#include <map>
#include <string>
#include <vector>
#include "common/common_status/exception/error_code_wrapper.h"
#include "common/common_status/exception/error_codes.h"
#include "common/common_status/status_or.h"
#include "common/io/hdf5/hdf5_exception.h"
#include "common/io/hdf5/hdf5_handles.h"
#include "common/io/hdf5/hdf5_options.h"
#include <H5Cpp.h>

namespace AIstudy {
namespace common {
namespace io {
namespace hdf5 {


/**
 * @brief HDF5 writer (straightforward impl, no factory/PIMPL).
 * Fallible operations return StatusOr<bool> only (no bool + err overloads).
 */
class Hdf5Writer {
public:
    Hdf5Writer() = default;
    ~Hdf5Writer();

    Hdf5Writer(const Hdf5Writer&) = delete;
    Hdf5Writer& operator=(const Hdf5Writer&) = delete;

    StatusOr<bool> createFile(const std::string& filepath);

    StatusOr<bool> openFile(const std::string& filepath, bool createIfNotExists = false);

    void closeFile();
    bool isFileOpen() const;
    const std::string& getFilePath() const;

    StatusOr<bool> writeDataset(const std::string& path, int value);
    StatusOr<bool> writeDataset(const std::string& path, double value);
    StatusOr<bool> writeDataset(const std::string& path, float value);
    StatusOr<bool> writeDataset(const std::string& path, const std::string& value);

    /** @brief 1D array (HDF5_IO §3.2). Writes std::vector as 1D dataset. */
    StatusOr<bool> writeDataset(const std::string& path, const std::vector<int>& vec);
    StatusOr<bool> writeDataset(const std::string& path, const std::vector<double>& vec);
    StatusOr<bool> writeDataset(const std::string& path, const std::vector<float>& vec);

    /** @brief Atomic write: tmp dataset then H5Lmove replace (HDF5_IO §6.2). Avoids partial write on crash. */
    StatusOr<bool> writeDatasetAtomic(const std::string& path, int value);
    StatusOr<bool> writeDatasetAtomic(const std::string& path, double value);
    StatusOr<bool> writeDatasetAtomic(const std::string& path, float value);
    StatusOr<bool> writeDatasetAtomic(const std::string& path, const std::string& value);
    StatusOr<bool> writeDatasetAtomic(const std::string& path, const std::vector<int>& vec);
    StatusOr<bool> writeDatasetAtomic(const std::string& path, const std::vector<double>& vec);
    StatusOr<bool> writeDatasetAtomic(const std::string& path, const std::vector<float>& vec);
    StatusOr<bool> writeDatasetAtomic(const std::string& path, const std::vector<int>& vec, const Hdf5WriteOptions& opts);
    StatusOr<bool> writeDatasetAtomic(const std::string& path, const std::vector<double>& vec, const Hdf5WriteOptions& opts);
    StatusOr<bool> writeDatasetAtomic(const std::string& path, const std::vector<float>& vec, const Hdf5WriteOptions& opts);
    StatusOr<bool> writeDatasetAtomic(const std::string& path, const std::vector<std::vector<int>>& mat);
    StatusOr<bool> writeDatasetAtomic(const std::string& path, const std::vector<std::vector<double>>& mat);
    StatusOr<bool> writeDatasetAtomic(const std::string& path, const std::vector<std::vector<float>>& mat);
    StatusOr<bool> writeDatasetAtomic(const std::string& path, const std::vector<std::vector<int>>& mat, const Hdf5WriteOptions& opts);
    StatusOr<bool> writeDatasetAtomic(const std::string& path, const std::vector<std::vector<double>>& mat, const Hdf5WriteOptions& opts);
    StatusOr<bool> writeDatasetAtomic(const std::string& path, const std::vector<std::vector<float>>& mat, const Hdf5WriteOptions& opts);
    StatusOr<bool> writeDatasetAtomic(const std::string& path, const std::vector<std::vector<std::vector<int>>>& cube);
    StatusOr<bool> writeDatasetAtomic(const std::string& path, const std::vector<std::vector<std::vector<double>>>& cube);
    StatusOr<bool> writeDatasetAtomic(const std::string& path, const std::vector<std::vector<std::vector<float>>>& cube);
    StatusOr<bool> writeDatasetAtomic(const std::string& path, const std::vector<std::string>& vec);

    /** @brief 1D with chunking/GZIP (HDF5_IO §4.2, §4.3). opts.chunkDims.size()==1; gzip 1-9 requires chunking. */
    StatusOr<bool> writeDataset(const std::string& path, const std::vector<int>& vec, const Hdf5WriteOptions& opts);
    StatusOr<bool> writeDataset(const std::string& path, const std::vector<double>& vec, const Hdf5WriteOptions& opts);
    StatusOr<bool> writeDataset(const std::string& path, const std::vector<float>& vec, const Hdf5WriteOptions& opts);

    /** @brief 2D array (HDF5_IO §3.2). Rectangular rows×cols; row-major. */
    StatusOr<bool> writeDataset(const std::string& path, const std::vector<std::vector<int>>& mat);
    StatusOr<bool> writeDataset(const std::string& path, const std::vector<std::vector<double>>& mat);
    StatusOr<bool> writeDataset(const std::string& path, const std::vector<std::vector<float>>& mat);

    /** @brief 2D with chunking/GZIP. opts.chunkDims.size()==2; gzip 1-9 requires chunking. */
    StatusOr<bool> writeDataset(const std::string& path, const std::vector<std::vector<int>>& mat, const Hdf5WriteOptions& opts);
    StatusOr<bool> writeDataset(const std::string& path, const std::vector<std::vector<double>>& mat, const Hdf5WriteOptions& opts);
    StatusOr<bool> writeDataset(const std::string& path, const std::vector<std::vector<float>>& mat, const Hdf5WriteOptions& opts);

    /** @brief 3D array (HDF5_IO §3.2). Rectangular dim0×dim1×dim2; row-major. */
    StatusOr<bool> writeDataset(const std::string& path, const std::vector<std::vector<std::vector<int>>>& cube);
    StatusOr<bool> writeDataset(const std::string& path, const std::vector<std::vector<std::vector<double>>>& cube);
    StatusOr<bool> writeDataset(const std::string& path, const std::vector<std::vector<std::vector<float>>>& cube);

    /** @brief 1D string array (HDF5_IO §3.3). Variable-length UTF-8. */
    StatusOr<bool> writeDataset(const std::string& path, const std::vector<std::string>& vec);

    StatusOr<bool> writeAttribute(const std::string& objectPath, const std::string& attrName, int value);
    StatusOr<bool> writeAttribute(const std::string& objectPath, const std::string& attrName, double value);
    StatusOr<bool> writeAttribute(const std::string& objectPath, const std::string& attrName, float value);
    StatusOr<bool> writeAttribute(const std::string& objectPath, const std::string& attrName, const std::string& value);

    /** @brief Batch scalar attributes (HDF5_IO §4.1). Fails on first error. */
    StatusOr<bool> writeAttributes(const std::string& objectPath, const std::map<std::string, int>& attrs);
    StatusOr<bool> writeAttributes(const std::string& objectPath, const std::map<std::string, double>& attrs);
    StatusOr<bool> writeAttributes(const std::string& objectPath, const std::map<std::string, std::string>& attrs);

    StatusOr<bool> createGroup(const std::string& path);

    /** @brief Batch write 1D double datasets (HDF5_IO §5.3). Fails on first error. */
    StatusOr<bool> writeDatasetsBatch(const std::map<std::string, std::vector<double>>& pathToData);

    /** @brief Delete node (group or dataset) at \a path. Fails if not found or invalid. */
    StatusOr<bool> deleteNode(const std::string& path);
    /** @brief Batch delete. Fails on first error. */
    StatusOr<bool> deleteNodes(const std::vector<std::string>& paths);

    /** @brief Append to 1D extensible double dataset (HDF5_IO §3.2, §4.4). Creates if not exist. */
    StatusOr<bool> appendDataset(const std::string& path, const std::vector<double>& data);

    void flush();
    void sync();

private:
    /** Ensure parent groups exist for dataset path (multi-level, HDF5_IO §5.2). */
    StatusOr<bool> ensureParentsForDataset(const std::string& path);

    /** Shared atomic write: write to path.tmp via \a writeToPath, then delete target if exists, H5Lmove tmp→path. */
    StatusOr<bool> atomicReplaceThenMove(const std::string& path,
                                         std::function<StatusOr<bool>(const std::string&)> writeToPath);

    std::string filepath_;
    H5FileHandle handle_;
};

} // namespace hdf5
} // namespace io
} // namespace common
} // namespace AIstudy


#endif // AISTUDY_HDF5_ENABLED

#endif // AISTUDY_COMMON_IO_HDF5_HDF5_WRITER_H
