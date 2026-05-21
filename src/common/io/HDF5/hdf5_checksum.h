/**
 * @file hdf5_checksum.h
 * @brief Checksum and consistency helpers for HDF5 datasets (HDF5_IO §4.4)
 *
 * writeDatasetWithChecksum / readDatasetWithVerify store and verify a 64-bit
 * checksum attribute on 1D double datasets.
 */

#ifndef AISTUDY_COMMON_IO_HDF5_HDF5_CHECKSUM_H
#define AISTUDY_COMMON_IO_HDF5_HDF5_CHECKSUM_H

#ifdef AISTUDY_HDF5_ENABLED

#include "common/status/status_or.h"
#include "common/io/hdf5/hdf5_reader.h"
#include "common/io/hdf5/hdf5_writer.h"
#include <cstdint>
#include <string>
#include <vector>

namespace AIstudy {
namespace common {
namespace io {
namespace hdf5 {


/** @brief Compute 64-bit FNV-1a style checksum over raw bytes of \a vec. */
uint64_t computeChecksum64(const std::vector<double>& vec);

/**
 * @brief Write 1D double dataset and store "checksum" attribute (hex string).
 * Uses existing writer; file must be open.
 */
StatusOr<bool> writeDatasetWithChecksum(Hdf5Writer& w, const std::string& path,
                                        const std::vector<double>& data);

/**
 * @brief Read 1D double dataset and verify "checksum" attribute.
 * If missing or mismatch, returns Fail(DATA_CORRUPT).
 * Optional \a expectedSize: if non-negative, fail with DATA_LENGTH_MISMATCH when size != expectedSize.
 */
StatusOr<std::vector<double>> readDatasetWithVerify(
    const Hdf5Reader& r, const std::string& path, int expectedSize = -1);

} // namespace hdf5
} // namespace io
} // namespace common
} // namespace AIstudy


#endif // AISTUDY_HDF5_ENABLED

#endif // AISTUDY_COMMON_IO_HDF5_HDF5_CHECKSUM_H
