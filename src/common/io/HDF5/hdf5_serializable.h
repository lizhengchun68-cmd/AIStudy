/**
 * @file hdf5_serializable.h
 * @brief ToHdf5/FromHdf5 protocol for struct serialization (HDF5_IO §3.5, §4.5)
 *
 * Protocol: T provides
 *   - StatusOr<bool> ToHdf5(Hdf5Writer& w, const std::string& basePath) const;
 *   - static StatusOr<T> FromHdf5(const Hdf5Reader& r, const std::string& basePath);
 * basePath is a group; scalars as attributes, arrays as datasets under basePath.
 */

#ifndef AISTUDY_COMMON_IO_HDF5_HDF5_SERIALIZABLE_H
#define AISTUDY_COMMON_IO_HDF5_HDF5_SERIALIZABLE_H

#ifdef AISTUDY_HDF5_ENABLED

#include "common/common_status/status_or.h"
#include "common/io/hdf5/hdf5_reader.h"
#include "common/io/hdf5/hdf5_writer.h"
#include <string>

namespace AIstudy {
namespace common {
namespace io {
namespace hdf5 {


/**
 * @brief Serialize T to HDF5 at \a objectPath. Opens \a filePath for write (create if needed).
 * T must implement ToHdf5(Hdf5Writer&, const std::string&) const.
 */
template <typename T>
StatusOr<bool> serializeToHdf5(const T& t, const std::string& filePath,
                               const std::string& objectPath) {
    Hdf5Writer w;
    auto open = w.openFile(filePath, true);
    if (!open.ok()) return open;
    return t.ToHdf5(w, objectPath);
}

/**
 * @brief Deserialize T from HDF5 at \a objectPath. Opens \a filePath for read.
 * T must implement static StatusOr<T> FromHdf5(const Hdf5Reader&, const std::string&).
 */
template <typename T>
StatusOr<T> deserializeFromHdf5StatusOr(const std::string& filePath,
                                        const std::string& objectPath) {
    Hdf5Reader r;
    auto open = r.openFile(filePath);
    if (!open.ok()) return StatusOr<T>::Fail(open.status());
    return T::FromHdf5(r, objectPath);
}

} // namespace hdf5
} // namespace io
} // namespace common
} // namespace AIstudy


#endif // AISTUDY_HDF5_ENABLED

#endif // AISTUDY_COMMON_IO_HDF5_HDF5_SERIALIZABLE_H
