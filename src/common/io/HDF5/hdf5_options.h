/**
 * @file hdf5_options.h
 * @brief Options for HDF5 dataset creation: chunking, GZIP (HDF5_IO §4.2, §4.3)
 */

#ifndef AISTUDY_COMMON_IO_HDF5_HDF5_OPTIONS_H
#define AISTUDY_COMMON_IO_HDF5_HDF5_OPTIONS_H

#ifdef AISTUDY_HDF5_ENABLED

#include <vector>
#include <H5Cpp.h>

namespace AIstudy {
namespace common {
namespace io {
namespace hdf5 {


/**
 * @brief Options for writeDataset when using chunking and/or GZIP.
 * - chunkDims: 1D => [n], 2D => [rows, cols]. Empty => no chunking.
 * - gzipLevel: -1 = off, 1-9 = GZIP. Requires chunking (non-empty chunkDims).
 */
struct Hdf5WriteOptions {
    std::vector<hsize_t> chunkDims;
    int gzipLevel = -1;
};

} // namespace hdf5
} // namespace io
} // namespace common
} // namespace AIstudy


#endif // AISTUDY_HDF5_ENABLED

#endif // AISTUDY_COMMON_IO_HDF5_HDF5_OPTIONS_H
