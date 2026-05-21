/**
 * @file hdf5_handles.h
 * @brief RAII handle wrappers for HDF5 resources (H5FileHandle, etc.)
 *
 * Requires AISTUDY_HDF5_ENABLED and HDF5 C++ library.
 */

#ifndef AISTUDY_COMMON_IO_HDF5_HDF5_HANDLES_H
#define AISTUDY_COMMON_IO_HDF5_HDF5_HANDLES_H

#ifdef AISTUDY_HDF5_ENABLED

#include <memory>
#include <string>
#include <H5Cpp.h>

namespace AIstudy {
namespace common {
namespace io {
namespace hdf5 {


/**
 * @brief RAII wrapper for HDF5 file handle. Move-only, no copy.
 * Construct -> open, destruct -> close. Invalid handle triggers HDF5_HANDLE_INVALID.
 */
class H5FileHandle {
public:
    H5FileHandle() = default;

    /** @brief Take ownership of \a p. Increments file handle count. */
    explicit H5FileHandle(std::unique_ptr<H5::H5File> p);

    ~H5FileHandle();
    H5FileHandle(H5FileHandle&&) noexcept = default;
    H5FileHandle& operator=(H5FileHandle&& other) noexcept;
    H5FileHandle(const H5FileHandle&) = delete;
    H5FileHandle& operator=(const H5FileHandle&) = delete;

    bool isValid() const noexcept { return file_ != nullptr; }
    H5::H5File* get() noexcept { return file_.get(); }
    const H5::H5File* get() const noexcept { return file_.get(); }
    /** @brief Raw file id for C API (e.g. H5Gopen2). Invalid => -1. */
    hid_t getId() const noexcept { return file_ ? file_->getId() : -1; }

    void flush() const;

private:
    std::unique_ptr<H5::H5File> file_;
};

/**
 * @brief RAII wrapper for HDF5 group. Move-only. Open via H5Gopen2, close in dtor.
 */
class H5GroupHandle {
public:
    H5GroupHandle() = default;
    H5GroupHandle(hid_t file_id, const std::string& path);
    ~H5GroupHandle();
    H5GroupHandle(H5GroupHandle&& other) noexcept;
    H5GroupHandle& operator=(H5GroupHandle&& other) noexcept;
    H5GroupHandle(const H5GroupHandle&) = delete;
    H5GroupHandle& operator=(const H5GroupHandle&) = delete;

    bool isValid() const noexcept { return gid_ >= 0; }
    hid_t get() const noexcept { return gid_; }

private:
    hid_t gid_ = -1;
};

/**
 * @brief RAII wrapper for HDF5 dataset. Move-only. Open via H5Dopen2, close in dtor.
 */
class H5DatasetHandle {
public:
    H5DatasetHandle() = default;
    H5DatasetHandle(hid_t file_id, const std::string& path);
    ~H5DatasetHandle();
    H5DatasetHandle(H5DatasetHandle&& other) noexcept;
    H5DatasetHandle& operator=(H5DatasetHandle&& other) noexcept;
    H5DatasetHandle(const H5DatasetHandle&) = delete;
    H5DatasetHandle& operator=(const H5DatasetHandle&) = delete;

    bool isValid() const noexcept { return did_ >= 0; }
    hid_t get() const noexcept { return did_; }

private:
    hid_t did_ = -1;
};

/**
 * @brief RAII wrapper for HDF5 dataspace. Move-only. Takes ownership of hid_t (e.g. from H5Dget_space).
 */
class H5DataspaceHandle {
public:
    H5DataspaceHandle() = default;
    explicit H5DataspaceHandle(hid_t sid);
    ~H5DataspaceHandle();
    H5DataspaceHandle(H5DataspaceHandle&& other) noexcept;
    H5DataspaceHandle& operator=(H5DataspaceHandle&& other) noexcept;
    H5DataspaceHandle(const H5DataspaceHandle&) = delete;
    H5DataspaceHandle& operator=(const H5DataspaceHandle&) = delete;

    bool isValid() const noexcept { return sid_ >= 0; }
    hid_t get() const noexcept { return sid_; }

private:
    hid_t sid_ = -1;
};

/** @brief Global count of open H5FileHandle instances (for leak checks in tests). */
int getH5FileHandleCount();

} // namespace hdf5
} // namespace io
} // namespace common
} // namespace AIstudy


#endif // AISTUDY_HDF5_ENABLED

#endif // AISTUDY_COMMON_IO_HDF5_HDF5_HANDLES_H
