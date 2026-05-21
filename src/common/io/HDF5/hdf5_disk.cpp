#ifdef AISTUDY_HDF5_ENABLED

#include "common/io/hdf5/hdf5_disk.h"
#include "common/common_status/exception/error_code_wrapper.h"
#include "common/common_status/exception/error_codes.h"
#include "common/common_status/exception/error_category.h"
#include <string>

#if defined(_WIN32) || defined(_WIN64)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <sys/statvfs.h>
#endif

namespace AIstudy {
namespace common {
namespace io {
namespace hdf5 {


namespace {

std::string dirOf(const std::string& path) {
    std::string::size_type i = path.find_last_of("/\\");
    if (i == std::string::npos) return ".";
    if (i == 0) return "/";
    return path.substr(0, i);
}

ErrorCodeWrapper makeHdf5Err(Hdf5Error e) {
    return ErrorCodeWrapper(static_cast<int>(e), ErrorCategory::HDF5);
}

} // namespace

#if defined(_WIN32) || defined(_WIN64)

StatusOr<uint64_t> getDiskFreeBytes(const std::string& path) {
    std::string dir = dirOf(path);
    if (dir.empty()) dir = ".";
    ULARGE_INTEGER freeAvail = {}, total = {}, freeTotal = {};
    if (!GetDiskFreeSpaceExA(dir.c_str(), &freeAvail, &total, &freeTotal))
        return StatusOr<uint64_t>::Fail(makeHdf5Err(Hdf5Error::DISK_FULL));
    return StatusOr<uint64_t>::Ok(static_cast<uint64_t>(freeAvail.QuadPart));
}

#else

StatusOr<uint64_t> getDiskFreeBytes(const std::string& path) {
    std::string dir = dirOf(path);
    if (dir.empty()) dir = ".";
    struct statvfs sv = {};
    if (statvfs(dir.c_str(), &sv) != 0)
        return StatusOr<uint64_t>::Fail(makeHdf5Err(Hdf5Error::DISK_FULL));
    uint64_t freeBytes = static_cast<uint64_t>(sv.f_bavail) * static_cast<uint64_t>(sv.f_frsize);
    return StatusOr<uint64_t>::Ok(freeBytes);
}

#endif

StatusOr<bool> checkDiskSpace(const std::string& path, uint64_t requiredBytes) {
    auto free = getDiskFreeBytes(path);
    if (!free.ok()) return StatusOr<bool>::Fail(free.status());
    if (free.value() < requiredBytes)
        return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::DISK_FULL));
    return StatusOr<bool>::Ok(true);
}

} // namespace hdf5
} // namespace io
} // namespace common
} // namespace AIstudy


#endif // AISTUDY_HDF5_ENABLED
