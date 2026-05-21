#ifdef AISTUDY_HDF5_ENABLED

#include "common/io/hdf5/hdf5_handles.h"
#include <H5Gpublic.h>
#include <H5Dpublic.h>
#include <H5Spublic.h>
#include <H5Ppublic.h>
#include <atomic>

namespace AIstudy {
namespace common {
namespace io {
namespace hdf5 {


namespace {

std::atomic<int>& fileHandleCount() {
    static std::atomic<int> n{0};
    return n;
}

std::string normPath(const std::string& path) {
    if (path.empty() || path[0] == '/') return path;
    return "/" + path;
}

} // namespace

H5FileHandle::H5FileHandle(std::unique_ptr<H5::H5File> p) : file_(std::move(p)) {
    if (file_)
        fileHandleCount()++;
}

H5FileHandle::~H5FileHandle() {
    if (file_)
        fileHandleCount()--;
}

H5FileHandle& H5FileHandle::operator=(H5FileHandle&& other) noexcept {
    if (this == &other) return *this;
    if (file_) fileHandleCount()--;
    file_ = std::move(other.file_);
    return *this;
}

void H5FileHandle::flush() const {
    if (file_)
        file_->flush(H5F_SCOPE_GLOBAL);
}

H5GroupHandle::H5GroupHandle(hid_t file_id, const std::string& path) {
    if (file_id < 0) return;
    std::string p = normPath(path);
    if (p.empty()) p = "/";
    gid_ = H5Gopen2(file_id, p.c_str(), H5P_DEFAULT);
}

H5GroupHandle::~H5GroupHandle() {
    if (gid_ >= 0) {
        H5Gclose(gid_);
        gid_ = -1;
    }
}

H5GroupHandle::H5GroupHandle(H5GroupHandle&& other) noexcept : gid_(other.gid_) {
    other.gid_ = -1;
}

H5GroupHandle& H5GroupHandle::operator=(H5GroupHandle&& other) noexcept {
    if (this == &other) return *this;
    if (gid_ >= 0) H5Gclose(gid_);
    gid_ = other.gid_;
    other.gid_ = -1;
    return *this;
}

H5DatasetHandle::H5DatasetHandle(hid_t file_id, const std::string& path) {
    if (file_id < 0) return;
    std::string p = normPath(path);
    if (p.empty()) return;
    did_ = H5Dopen2(file_id, p.c_str(), H5P_DEFAULT);
}

H5DatasetHandle::~H5DatasetHandle() {
    if (did_ >= 0) {
        H5Dclose(did_);
        did_ = -1;
    }
}

H5DatasetHandle::H5DatasetHandle(H5DatasetHandle&& other) noexcept : did_(other.did_) {
    other.did_ = -1;
}

H5DatasetHandle& H5DatasetHandle::operator=(H5DatasetHandle&& other) noexcept {
    if (this == &other) return *this;
    if (did_ >= 0) H5Dclose(did_);
    did_ = other.did_;
    other.did_ = -1;
    return *this;
}

H5DataspaceHandle::H5DataspaceHandle(hid_t sid) : sid_(sid) {}

H5DataspaceHandle::~H5DataspaceHandle() {
    if (sid_ >= 0) {
        H5Sclose(sid_);
        sid_ = -1;
    }
}

H5DataspaceHandle::H5DataspaceHandle(H5DataspaceHandle&& other) noexcept : sid_(other.sid_) {
    other.sid_ = -1;
}

H5DataspaceHandle& H5DataspaceHandle::operator=(H5DataspaceHandle&& other) noexcept {
    if (this == &other) return *this;
    if (sid_ >= 0) H5Sclose(sid_);
    sid_ = other.sid_;
    other.sid_ = -1;
    return *this;
}

int getH5FileHandleCount() {
    return fileHandleCount().load();
}

} // namespace hdf5
} // namespace io
} // namespace common
} // namespace AIstudy


#endif // AISTUDY_HDF5_ENABLED
