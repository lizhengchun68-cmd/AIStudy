#ifdef AISTUDY_HDF5_ENABLED

#include "common/io/hdf5/hdf5_file_lock.h"
#include "common/status/exception/error_code_wrapper.h"
#include "common/status/exception/error_codes.h"
#include "common/status/exception/error_category.h"
#include <cerrno>
#include <chrono>
#include <thread>

#if defined(_WIN32) || defined(_WIN64)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif

namespace AIstudy {
namespace common {
namespace io {
namespace hdf5 {


namespace {

inline std::string lockPath(const std::string& hdf5Path) {
    return hdf5Path + ".lock";
}

ErrorCodeWrapper makeHdf5Err(Hdf5Error e) {
    return ErrorCodeWrapper(static_cast<int>(e), ErrorCategory::HDF5);
}

} // namespace

#if defined(_WIN32) || defined(_WIN64)

FileLock::~FileLock() {
    if (fd_ < 0) return;
    HANDLE h = reinterpret_cast<HANDLE>(_get_osfhandle(fd_));
    if (h != INVALID_HANDLE_VALUE) {
        OVERLAPPED ov = {};
        UnlockFileEx(h, 0, 1, 0, &ov);
    }
    ::_close(fd_);
    fd_ = -1;
}

StatusOr<std::unique_ptr<FileLock>> FileLock::tryLock(const std::string& hdf5Path, int timeoutMs) {
    std::string path = lockPath(hdf5Path);
    int fd = ::_open(path.c_str(), _O_RDWR | _O_CREAT, _S_IREAD | _S_IWRITE);
    if (fd < 0)
        return StatusOr<std::unique_ptr<FileLock>>::Fail(makeHdf5Err(Hdf5Error::FILE_OPEN_ERROR));
    HANDLE h = reinterpret_cast<HANDLE>(_get_osfhandle(fd));
    if (h == INVALID_HANDLE_VALUE) {
        ::_close(fd);
        return StatusOr<std::unique_ptr<FileLock>>::Fail(makeHdf5Err(Hdf5Error::FILE_OPEN_ERROR));
    }
    OVERLAPPED ov = {};
    auto start = std::chrono::steady_clock::now();
    for (;;) {
        BOOL ok = LockFileEx(h, LOCKFILE_EXCLUSIVE_LOCK | LOCKFILE_FAIL_IMMEDIATELY, 0, 1, 0, &ov);
        if (ok) {
            auto out = std::unique_ptr<FileLock>(new FileLock(fd));
            return StatusOr<std::unique_ptr<FileLock>>::Ok(std::move(out));
        }
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start).count();
        if (elapsed >= timeoutMs) {
            ::_close(fd);
            return StatusOr<std::unique_ptr<FileLock>>::Fail(makeHdf5Err(Hdf5Error::FILE_LOCK_TIMEOUT));
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

#else

FileLock::~FileLock() {
    if (fd_ < 0) return;
    struct flock fl = {};
    fl.l_type = F_UNLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;
    fcntl(fd_, F_SETLK, &fl);
    ::close(fd_);
    fd_ = -1;
}

StatusOr<std::unique_ptr<FileLock>> FileLock::tryLock(const std::string& hdf5Path, int timeoutMs) {
    std::string path = lockPath(hdf5Path);
    int fd = ::open(path.c_str(), O_RDWR | O_CREAT, 0666);
    if (fd < 0)
        return StatusOr<std::unique_ptr<FileLock>>::Fail(makeHdf5Err(Hdf5Error::FILE_OPEN_ERROR));
    struct flock fl = {};
    fl.l_type = F_WRLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;
    auto start = std::chrono::steady_clock::now();
    for (;;) {
        int r = fcntl(fd, F_SETLK, &fl);
        if (r == 0) {
            auto out = std::unique_ptr<FileLock>(new FileLock(fd));
            return StatusOr<std::unique_ptr<FileLock>>::Ok(std::move(out));
        }
        if (errno != EAGAIN && errno != EACCES) {
            ::close(fd);
            return StatusOr<std::unique_ptr<FileLock>>::Fail(makeHdf5Err(Hdf5Error::FILE_OPEN_ERROR));
        }
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start).count();
        if (elapsed >= timeoutMs) {
            ::close(fd);
            return StatusOr<std::unique_ptr<FileLock>>::Fail(makeHdf5Err(Hdf5Error::FILE_LOCK_TIMEOUT));
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

#endif

} // namespace hdf5
} // namespace io
} // namespace common
} // namespace AIstudy


#endif // AISTUDY_HDF5_ENABLED
