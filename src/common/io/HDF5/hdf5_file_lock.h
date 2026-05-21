/**
 * @file hdf5_file_lock.h
 * @brief File-level exclusive lock for HDF5 multi-process safety (HDF5_IO §6.4)
 *
 * Uses a sidecar .lock file. RAII: hold FileLock during create/open/write, release on destroy.
 * Linux: fcntl F_WRLCK; Windows: LockFileEx.
 */

#ifndef AISTUDY_COMMON_IO_HDF5_HDF5_FILE_LOCK_H
#define AISTUDY_COMMON_IO_HDF5_HDF5_FILE_LOCK_H

#ifdef AISTUDY_HDF5_ENABLED

#include "common/common_status/status_or.h"
#include <memory>
#include <string>

namespace AIstudy {
namespace common {
namespace io {
namespace hdf5 {


/**
 * @brief RAII file lock. Unlocks and closes on destruction.
 */
class FileLock {
public:
    FileLock() = default;
    ~FileLock();

    FileLock(FileLock&& other) noexcept : fd_(other.fd_) { other.fd_ = -1; }
    FileLock& operator=(FileLock&& other) noexcept {
        if (this != &other) {
            FileLock tmp(std::move(other));
            std::swap(fd_, tmp.fd_);
        }
        return *this;
    }
    FileLock(const FileLock&) = delete;
    FileLock& operator=(const FileLock&) = delete;

    /** @brief Try to acquire exclusive lock on sidecar \a hdf5Path + ".lock".
     * Retries up to \a timeoutMs. Returns Fail(FILE_LOCK_TIMEOUT) on timeout. */
    static StatusOr<std::unique_ptr<FileLock>> tryLock(const std::string& hdf5Path, int timeoutMs = 5000);

    bool isLocked() const { return fd_ >= 0; }

private:
    explicit FileLock(int fd) : fd_(fd) {}
    int fd_ = -1;
};

} // namespace hdf5
} // namespace io
} // namespace common
} // namespace AIstudy


#endif // AISTUDY_HDF5_ENABLED

#endif // AISTUDY_COMMON_IO_HDF5_HDF5_FILE_LOCK_H
