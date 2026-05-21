#ifdef AISTUDY_HDF5_ENABLED

#include "common/io/hdf5/hdf5_writer.h"
#include "common/status/exception/error_category.h"
#include "common/status/status_or.h"
#include "common/io/hdf5/hdf5_handles.h"
#include "common/io/hdf5/hdf5_logging.h"
#include "common/io/hdf5/hdf5_options.h"
#include <H5Cpp.h>
#include <H5Exception.h>
#include <H5Lpublic.h>
#include <H5Ppublic.h>
#include <chrono>
#include <functional>
#include <memory>
#include <thread>

namespace AIstudy {
namespace common {
namespace io {
namespace hdf5 {


namespace {

constexpr int kRetryMaxAttempts = 3;
constexpr int kRetryBaseDelayMs = 50;

ErrorCodeWrapper makeHdf5Err(Hdf5Error e) {
    return ErrorCodeWrapper(static_cast<int>(e), ErrorCategory::HDF5);
}

void retrySleep(int attempt) {
    if (attempt >= kRetryMaxAttempts - 1) return;
    int ms = kRetryBaseDelayMs * (1 << attempt);
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

std::string normPath(const std::string& path) {
    if (path.empty() || path[0] == '/') return path;
    return "/" + path;
}

/** Parent of dataset path for multi-level create (HDF5_IO §5.2). e.g. /a/b/c -> /a/b. */
std::string parentPath(const std::string& path) {
    std::string p = normPath(path);
    std::string::size_type i = p.rfind('/');
    if (i == std::string::npos || i == 0) return "";
    return p.substr(0, static_cast<size_t>(i));
}

/** Last path component for delete. e.g. /a/b/c -> c, /a -> a. */
std::string linkName(const std::string& path) {
    std::string p = normPath(path);
    if (p.empty() || p == "/") return "";
    std::string::size_type i = p.rfind('/');
    if (i == std::string::npos) return p;
    return p.substr(i + 1);
}

bool datasetExistsAt(H5::H5File* fp, const std::string& path) {
    if (!fp) return false;
    try {
        std::string p = normPath(path);
        H5::DataSet d = fp->openDataSet(p);
        d.close();
        return true;
    } catch (...) { return false; }
}

template <typename F>
void withObjForAttr(H5::H5File* file, const std::string& path, F f) {
    std::string p = normPath(path);
    if (p.empty() || p == "/") { f(file); return; }
    try {
        H5::DataSet d = file->openDataSet(p);
        f(&d);
        return;
    } catch (...) {}
    H5::Group g = file->openGroup(p);
    f(&g);
}

H5::DSetCreatPropList makePlist1D(const Hdf5WriteOptions& opts, hsize_t) {
    if (opts.gzipLevel >= 1 && opts.gzipLevel <= 9 && opts.chunkDims.empty())
        throw std::invalid_argument("gzip requires chunking");
    if (opts.chunkDims.size() != 1 || opts.chunkDims[0] == 0)
        throw std::invalid_argument("chunkDims must be [n] for 1D");
    H5::DSetCreatPropList plist;
    plist.setChunk(1, opts.chunkDims.data());
    if (opts.gzipLevel >= 1 && opts.gzipLevel <= 9)
        plist.setDeflate(opts.gzipLevel);
    return plist;
}

H5::DSetCreatPropList makePlist2D(const Hdf5WriteOptions& opts) {
    if (opts.gzipLevel >= 1 && opts.gzipLevel <= 9 && opts.chunkDims.empty())
        throw std::invalid_argument("gzip requires chunking");
    if (opts.chunkDims.size() != 2 || opts.chunkDims[0] == 0 || opts.chunkDims[1] == 0)
        throw std::invalid_argument("chunkDims must be [r,c] for 2D");
    H5::DSetCreatPropList plist;
    plist.setChunk(2, opts.chunkDims.data());
    if (opts.gzipLevel >= 1 && opts.gzipLevel <= 9)
        plist.setDeflate(opts.gzipLevel);
    return plist;
}

} // namespace

StatusOr<bool> Hdf5Writer::ensureParentsForDataset(const std::string& path) {
    std::string parent = parentPath(path);
    if (parent.empty()) return StatusOr<bool>::Ok(true);
    return createGroup(parent);
}

Hdf5Writer::~Hdf5Writer() { closeFile(); }

void Hdf5Writer::closeFile() {
    std::string p = filepath_.empty() ? "(none)" : filepath_;
    Hdf5OpTrace trace("closeFile", p);
    handle_ = H5FileHandle();
    filepath_.clear();
}

bool Hdf5Writer::isFileOpen() const { return handle_.isValid(); }

const std::string& Hdf5Writer::getFilePath() const { return filepath_; }

StatusOr<bool> Hdf5Writer::createFile(const std::string& filepath) {
    Hdf5OpTrace trace("createFile", filepath);
    closeFile();
    H5::Exception::dontPrint();
    StatusOr<bool> last = StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::FILE_CREATE_ERROR));
    for (int attempt = 0; attempt < kRetryMaxAttempts; ++attempt) {
        try {
            auto p = std::unique_ptr<H5::H5File>(new H5::H5File(filepath, H5F_ACC_TRUNC));
            handle_ = H5FileHandle(std::move(p));
            filepath_ = filepath;
            return StatusOr<bool>::Ok(true);
        } catch (const H5::FileIException&) {
            last = StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::FILE_CREATE_ERROR));
            retrySleep(attempt);
        } catch (const H5::Exception&) {
            last = StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::FILE_CREATE_ERROR));
            retrySleep(attempt);
        }
    }
    trace.fail("FILE_CREATE_ERROR");
    return last;
}

StatusOr<bool> Hdf5Writer::openFile(const std::string& filepath, bool createIfNotExists) {
    Hdf5OpTrace trace("openFile", filepath);
    closeFile();
    H5::Exception::dontPrint();
    try {
        auto p = std::unique_ptr<H5::H5File>(new H5::H5File(filepath, H5F_ACC_RDWR));
        handle_ = H5FileHandle(std::move(p));
        filepath_ = filepath;
        return StatusOr<bool>::Ok(true);
    } catch (const H5::FileIException&) {
        if (createIfNotExists) {
            try {
                auto q = std::unique_ptr<H5::H5File>(new H5::H5File(filepath, H5F_ACC_TRUNC));
                handle_ = H5FileHandle(std::move(q));
                filepath_ = filepath;
                return StatusOr<bool>::Ok(true);
            } catch (const H5::Exception&) {
                trace.fail("FILE_CREATE_ERROR");
                return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::FILE_CREATE_ERROR));
            }
        }
        trace.fail("FILE_OPEN_ERROR");
        return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::FILE_OPEN_ERROR));
    } catch (const H5::Exception&) {
        trace.fail("FILE_OPEN_ERROR");
        return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::FILE_OPEN_ERROR));
    }
}

void Hdf5Writer::flush() {
    if (handle_.get()) handle_.get()->flush(H5F_SCOPE_LOCAL);
}

void Hdf5Writer::sync() {
    if (handle_.get()) handle_.get()->flush(H5F_SCOPE_GLOBAL);
}

StatusOr<bool> Hdf5Writer::createGroup(const std::string& path) {
    Hdf5OpTrace trace("createGroup", path);
    if (!handle_.isValid()) {
        trace.fail("HANDLE_INVALID");
        return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::HANDLE_INVALID));
    }
    H5::H5File* fp = handle_.get();
    std::string p = normPath(path);
    if (p.empty() || p == "/") return StatusOr<bool>::Ok(true);
    try {
        for (size_t i = 1; i <= p.size(); ++i) {
            if (i == p.size() || p[i] == '/') {
                std::string seg = p.substr(0, i);
                if (seg.empty()) continue;
                try { fp->openGroup(seg); } catch (...) { fp->createGroup(seg); }
            }
        }
        return StatusOr<bool>::Ok(true);
    } catch (const H5::GroupIException&) {
        trace.fail("GROUP_CREATE_ERROR");
        return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::GROUP_CREATE_ERROR));
    } catch (const H5::Exception&) {
        trace.fail("GROUP_CREATE_ERROR");
        return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::GROUP_CREATE_ERROR));
    }
}

StatusOr<bool> Hdf5Writer::writeDataset(const std::string& path, int value) {
    if (!handle_.isValid()) return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::HANDLE_INVALID));
    { auto e = ensureParentsForDataset(path); if (!e.ok()) return e; }
    H5::H5File* fp = handle_.get();
    std::string p = normPath(path);
    try {
        H5::DataSpace sp(H5S_SCALAR);
        H5::DataSet ds = fp->createDataSet(p, H5::PredType::NATIVE_INT, sp);
        ds.write(&value, H5::PredType::NATIVE_INT);
        return StatusOr<bool>::Ok(true);
    } catch (const H5::DataSetIException&) {
        return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::DATASET_WRITE_ERROR));
    } catch (const H5::Exception&) {
        return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::DATASET_WRITE_ERROR));
    }
}

StatusOr<bool> Hdf5Writer::writeDataset(const std::string& path, double value) {
    if (!handle_.isValid()) return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::HANDLE_INVALID));
    { auto e = ensureParentsForDataset(path); if (!e.ok()) return e; }
    H5::H5File* fp = handle_.get();
    std::string p = normPath(path);
    try {
        H5::DataSpace sp(H5S_SCALAR);
        H5::DataSet ds = fp->createDataSet(p, H5::PredType::NATIVE_DOUBLE, sp);
        ds.write(&value, H5::PredType::NATIVE_DOUBLE);
        return StatusOr<bool>::Ok(true);
    } catch (const H5::DataSetIException&) {
        return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::DATASET_WRITE_ERROR));
    } catch (const H5::Exception&) {
        return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::DATASET_WRITE_ERROR));
    }
}

StatusOr<bool> Hdf5Writer::writeDataset(const std::string& path, float value) {
    if (!handle_.isValid()) return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::HANDLE_INVALID));
    { auto e = ensureParentsForDataset(path); if (!e.ok()) return e; }
    H5::H5File* fp = handle_.get();
    std::string p = normPath(path);
    try {
        H5::DataSpace sp(H5S_SCALAR);
        H5::DataSet ds = fp->createDataSet(p, H5::PredType::NATIVE_FLOAT, sp);
        ds.write(&value, H5::PredType::NATIVE_FLOAT);
        return StatusOr<bool>::Ok(true);
    } catch (const H5::DataSetIException&) {
        return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::DATASET_WRITE_ERROR));
    } catch (const H5::Exception&) {
        return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::DATASET_WRITE_ERROR));
    }
}

StatusOr<bool> Hdf5Writer::writeDataset(const std::string& path, const std::string& value) {
    if (!handle_.isValid()) return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::HANDLE_INVALID));
    { auto e = ensureParentsForDataset(path); if (!e.ok()) return e; }
    H5::H5File* fp = handle_.get();
    std::string p = normPath(path);
    try {
        H5::StrType st(H5::PredType::C_S1, H5T_VARIABLE);
        H5::DataSpace sp(H5S_SCALAR);
        H5::DataSet ds = fp->createDataSet(p, st, sp);
        H5std_string s(value);
        ds.write(s, st);
        return StatusOr<bool>::Ok(true);
    } catch (const H5::DataSetIException&) {
        return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::DATASET_WRITE_ERROR));
    } catch (const H5::Exception&) {
        return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::DATASET_WRITE_ERROR));
    }
}

StatusOr<bool> Hdf5Writer::writeDataset(const std::string& path, const std::vector<int>& vec) {
    if (!handle_.isValid()) return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::HANDLE_INVALID));
    { auto e = ensureParentsForDataset(path); if (!e.ok()) return e; }
    H5::H5File* fp = handle_.get();
    std::string p = normPath(path);
    try {
        hsize_t dims[1] = { vec.size() };
        H5::DataSpace sp(1, dims);
        H5::DataSet ds = fp->createDataSet(p, H5::PredType::NATIVE_INT, sp);
        if (!vec.empty())
            ds.write(vec.data(), H5::PredType::NATIVE_INT);
        return StatusOr<bool>::Ok(true);
    } catch (const H5::DataSetIException&) {
        return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::DATASET_WRITE_ERROR));
    } catch (const H5::Exception&) {
        return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::DATASET_WRITE_ERROR));
    }
}

StatusOr<bool> Hdf5Writer::writeDataset(const std::string& path, const std::vector<double>& vec) {
    Hdf5OpTrace trace("writeDataset", path);
    if (!handle_.isValid()) {
        trace.fail("HANDLE_INVALID");
        return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::HANDLE_INVALID));
    }
    { auto e = ensureParentsForDataset(path); if (!e.ok()) { trace.fail("ensureParents"); return e; } }
    H5::H5File* fp = handle_.get();
    std::string p = normPath(path);
    StatusOr<bool> last = StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::DATASET_WRITE_ERROR));
    for (int attempt = 0; attempt < kRetryMaxAttempts; ++attempt) {
        try {
            trace.setDetail("len=" + std::to_string(vec.size()));
            hsize_t dims[1] = { vec.size() };
            H5::DataSpace sp(1, dims);
            H5::DataSet ds = fp->createDataSet(p, H5::PredType::NATIVE_DOUBLE, sp);
            if (!vec.empty())
                ds.write(vec.data(), H5::PredType::NATIVE_DOUBLE);
            return StatusOr<bool>::Ok(true);
        } catch (const H5::DataSetIException&) {
            last = StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::DATASET_WRITE_ERROR));
            retrySleep(attempt);
        } catch (const H5::Exception&) {
            last = StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::DATASET_WRITE_ERROR));
            retrySleep(attempt);
        }
    }
    trace.fail("DATASET_WRITE_ERROR");
    return last;
}

StatusOr<bool> Hdf5Writer::atomicReplaceThenMove(const std::string& path,
                                                 std::function<StatusOr<bool>(const std::string&)> writeToPath) {
    Hdf5OpTrace trace("writeDatasetAtomic", path);
    if (!handle_.isValid()) {
        trace.fail("HANDLE_INVALID");
        return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::HANDLE_INVALID));
    }
    { auto e = ensureParentsForDataset(path); if (!e.ok()) { trace.fail("ensureParents"); return e; } }
    const std::string p = normPath(path);
    const std::string tmpPath = p + ".tmp";
    auto wr = writeToPath(tmpPath);
    if (!wr.ok()) {
        trace.fail("tmp write failed");
        return wr;
    }
    H5::H5File* fp = handle_.get();
    try {
        if (datasetExistsAt(fp, p)) {
            auto del = deleteNode(path);
            if (!del.ok()) {
                trace.fail("deleteNode");
                return del;
            }
        }
        hid_t fid = handle_.getId();
        if (fid < 0) {
            trace.fail("HANDLE_INVALID");
            return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::HANDLE_INVALID));
        }
        herr_t err = H5Lmove(fid, tmpPath.c_str(), fid, p.c_str(), H5P_DEFAULT, H5P_DEFAULT);
        if (err < 0) {
            trace.fail("H5Lmove");
            return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::DATASET_WRITE_ERROR));
        }
        return StatusOr<bool>::Ok(true);
    } catch (const H5::Exception&) {
        trace.fail("atomic replace");
        return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::DATASET_WRITE_ERROR));
    }
}

StatusOr<bool> Hdf5Writer::writeDatasetAtomic(const std::string& path, int value) {
    return atomicReplaceThenMove(path, [this, value](const std::string& p) { return writeDataset(p, value); });
}
StatusOr<bool> Hdf5Writer::writeDatasetAtomic(const std::string& path, double value) {
    return atomicReplaceThenMove(path, [this, value](const std::string& p) { return writeDataset(p, value); });
}
StatusOr<bool> Hdf5Writer::writeDatasetAtomic(const std::string& path, float value) {
    return atomicReplaceThenMove(path, [this, value](const std::string& p) { return writeDataset(p, value); });
}
StatusOr<bool> Hdf5Writer::writeDatasetAtomic(const std::string& path, const std::string& value) {
    return atomicReplaceThenMove(path, [this, &value](const std::string& p) { return writeDataset(p, value); });
}
StatusOr<bool> Hdf5Writer::writeDatasetAtomic(const std::string& path, const std::vector<int>& vec) {
    return atomicReplaceThenMove(path, [this, &vec](const std::string& p) { return writeDataset(p, vec); });
}
StatusOr<bool> Hdf5Writer::writeDatasetAtomic(const std::string& path, const std::vector<double>& vec) {
    return atomicReplaceThenMove(path, [this, &vec](const std::string& p) { return writeDataset(p, vec); });
}
StatusOr<bool> Hdf5Writer::writeDatasetAtomic(const std::string& path, const std::vector<float>& vec) {
    return atomicReplaceThenMove(path, [this, &vec](const std::string& p) { return writeDataset(p, vec); });
}
StatusOr<bool> Hdf5Writer::writeDatasetAtomic(const std::string& path, const std::vector<int>& vec, const Hdf5WriteOptions& opts) {
    return atomicReplaceThenMove(path, [this, &vec, &opts](const std::string& p) { return writeDataset(p, vec, opts); });
}
StatusOr<bool> Hdf5Writer::writeDatasetAtomic(const std::string& path, const std::vector<double>& vec, const Hdf5WriteOptions& opts) {
    return atomicReplaceThenMove(path, [this, &vec, &opts](const std::string& p) { return writeDataset(p, vec, opts); });
}
StatusOr<bool> Hdf5Writer::writeDatasetAtomic(const std::string& path, const std::vector<float>& vec, const Hdf5WriteOptions& opts) {
    return atomicReplaceThenMove(path, [this, &vec, &opts](const std::string& p) { return writeDataset(p, vec, opts); });
}
StatusOr<bool> Hdf5Writer::writeDatasetAtomic(const std::string& path, const std::vector<std::vector<int>>& mat) {
    return atomicReplaceThenMove(path, [this, &mat](const std::string& p) { return writeDataset(p, mat); });
}
StatusOr<bool> Hdf5Writer::writeDatasetAtomic(const std::string& path, const std::vector<std::vector<double>>& mat) {
    return atomicReplaceThenMove(path, [this, &mat](const std::string& p) { return writeDataset(p, mat); });
}
StatusOr<bool> Hdf5Writer::writeDatasetAtomic(const std::string& path, const std::vector<std::vector<float>>& mat) {
    return atomicReplaceThenMove(path, [this, &mat](const std::string& p) { return writeDataset(p, mat); });
}
StatusOr<bool> Hdf5Writer::writeDatasetAtomic(const std::string& path, const std::vector<std::vector<int>>& mat, const Hdf5WriteOptions& opts) {
    return atomicReplaceThenMove(path, [this, &mat, &opts](const std::string& p) { return writeDataset(p, mat, opts); });
}
StatusOr<bool> Hdf5Writer::writeDatasetAtomic(const std::string& path, const std::vector<std::vector<double>>& mat, const Hdf5WriteOptions& opts) {
    return atomicReplaceThenMove(path, [this, &mat, &opts](const std::string& p) { return writeDataset(p, mat, opts); });
}
StatusOr<bool> Hdf5Writer::writeDatasetAtomic(const std::string& path, const std::vector<std::vector<float>>& mat, const Hdf5WriteOptions& opts) {
    return atomicReplaceThenMove(path, [this, &mat, &opts](const std::string& p) { return writeDataset(p, mat, opts); });
}
StatusOr<bool> Hdf5Writer::writeDatasetAtomic(const std::string& path, const std::vector<std::vector<std::vector<int>>>& cube) {
    return atomicReplaceThenMove(path, [this, &cube](const std::string& p) { return writeDataset(p, cube); });
}
StatusOr<bool> Hdf5Writer::writeDatasetAtomic(const std::string& path, const std::vector<std::vector<std::vector<double>>>& cube) {
    return atomicReplaceThenMove(path, [this, &cube](const std::string& p) { return writeDataset(p, cube); });
}
StatusOr<bool> Hdf5Writer::writeDatasetAtomic(const std::string& path, const std::vector<std::vector<std::vector<float>>>& cube) {
    return atomicReplaceThenMove(path, [this, &cube](const std::string& p) { return writeDataset(p, cube); });
}
StatusOr<bool> Hdf5Writer::writeDatasetAtomic(const std::string& path, const std::vector<std::string>& vec) {
    return atomicReplaceThenMove(path, [this, &vec](const std::string& p) { return writeDataset(p, vec); });
}

StatusOr<bool> Hdf5Writer::writeDataset(const std::string& path, const std::vector<float>& vec) {
    if (!handle_.isValid()) return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::HANDLE_INVALID));
    { auto e = ensureParentsForDataset(path); if (!e.ok()) return e; }
    H5::H5File* fp = handle_.get();
    std::string p = normPath(path);
    try {
        hsize_t dims[1] = { vec.size() };
        H5::DataSpace sp(1, dims);
        H5::DataSet ds = fp->createDataSet(p, H5::PredType::NATIVE_FLOAT, sp);
        if (!vec.empty())
            ds.write(vec.data(), H5::PredType::NATIVE_FLOAT);
        return StatusOr<bool>::Ok(true);
    } catch (const H5::DataSetIException&) {
        return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::DATASET_WRITE_ERROR));
    } catch (const H5::Exception&) {
        return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::DATASET_WRITE_ERROR));
    }
}

StatusOr<bool> Hdf5Writer::writeDataset(const std::string& path, const std::vector<int>& vec, const Hdf5WriteOptions& opts) {
    if (!handle_.isValid()) return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::HANDLE_INVALID));
    if (opts.chunkDims.empty() && opts.gzipLevel < 0)
        return writeDataset(path, vec);
    { auto e = ensureParentsForDataset(path); if (!e.ok()) return e; }
    H5::H5File* fp = handle_.get();
    std::string p = normPath(path);
    try {
        H5::DSetCreatPropList plist = makePlist1D(opts, vec.size());
        hsize_t dims[1] = { vec.size() };
        H5::DataSpace sp(1, dims);
        H5::DataSet ds = fp->createDataSet(p, H5::PredType::NATIVE_INT, sp, plist);
        if (!vec.empty())
            ds.write(vec.data(), H5::PredType::NATIVE_INT);
        return StatusOr<bool>::Ok(true);
    } catch (const std::invalid_argument&) {
        return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::COMPRESSION_ERROR));
    } catch (const H5::DataSetIException&) {
        return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::DATASET_WRITE_ERROR));
    } catch (const H5::Exception&) {
        return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::DATASET_WRITE_ERROR));
    }
}

StatusOr<bool> Hdf5Writer::writeDataset(const std::string& path, const std::vector<double>& vec, const Hdf5WriteOptions& opts) {
    if (!handle_.isValid()) return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::HANDLE_INVALID));
    if (opts.chunkDims.empty() && opts.gzipLevel < 0)
        return writeDataset(path, vec);
    { auto e = ensureParentsForDataset(path); if (!e.ok()) return e; }
    H5::H5File* fp = handle_.get();
    std::string p = normPath(path);
    H5::DSetCreatPropList plist;
    try {
        plist = makePlist1D(opts, vec.size());
    } catch (const std::invalid_argument&) {
        return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::COMPRESSION_ERROR));
    }
    StatusOr<bool> last = StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::DATASET_WRITE_ERROR));
    for (int attempt = 0; attempt < kRetryMaxAttempts; ++attempt) {
        try {
            hsize_t dims[1] = { vec.size() };
            H5::DataSpace sp(1, dims);
            H5::DataSet ds = fp->createDataSet(p, H5::PredType::NATIVE_DOUBLE, sp, plist);
            if (!vec.empty())
                ds.write(vec.data(), H5::PredType::NATIVE_DOUBLE);
            return StatusOr<bool>::Ok(true);
        } catch (const H5::DataSetIException&) {
            last = StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::DATASET_WRITE_ERROR));
            retrySleep(attempt);
        } catch (const H5::Exception&) {
            last = StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::DATASET_WRITE_ERROR));
            retrySleep(attempt);
        }
    }
    return last;
}

StatusOr<bool> Hdf5Writer::writeDataset(const std::string& path, const std::vector<float>& vec, const Hdf5WriteOptions& opts) {
    if (!handle_.isValid()) return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::HANDLE_INVALID));
    if (opts.chunkDims.empty() && opts.gzipLevel < 0)
        return writeDataset(path, vec);
    { auto e = ensureParentsForDataset(path); if (!e.ok()) return e; }
    H5::H5File* fp = handle_.get();
    std::string p = normPath(path);
    try {
        H5::DSetCreatPropList plist = makePlist1D(opts, vec.size());
        hsize_t dims[1] = { vec.size() };
        H5::DataSpace sp(1, dims);
        H5::DataSet ds = fp->createDataSet(p, H5::PredType::NATIVE_FLOAT, sp, plist);
        if (!vec.empty())
            ds.write(vec.data(), H5::PredType::NATIVE_FLOAT);
        return StatusOr<bool>::Ok(true);
    } catch (const std::invalid_argument&) {
        return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::COMPRESSION_ERROR));
    } catch (const H5::DataSetIException&) {
        return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::DATASET_WRITE_ERROR));
    } catch (const H5::Exception&) {
        return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::DATASET_WRITE_ERROR));
    }
}

StatusOr<bool> Hdf5Writer::writeDataset(const std::string& path, const std::vector<std::vector<int>>& mat) {
    if (!handle_.isValid()) return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::HANDLE_INVALID));
    { auto e = ensureParentsForDataset(path); if (!e.ok()) return e; }
    H5::H5File* fp = handle_.get();
    std::string p = normPath(path);
    try {
        const size_t rows = mat.size();
        if (rows == 0) {
            hsize_t dims[2] = {0, 0};
            H5::DataSpace sp(2, dims);
            fp->createDataSet(p, H5::PredType::NATIVE_INT, sp);
            return StatusOr<bool>::Ok(true);
        }
        const size_t cols = mat[0].size();
        for (size_t i = 1; i < rows; ++i)
            if (mat[i].size() != cols)
                return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::DATASET_WRITE_ERROR));
        hsize_t dims[2] = {rows, cols};
        H5::DataSpace sp(2, dims);
        H5::DataSet ds = fp->createDataSet(p, H5::PredType::NATIVE_INT, sp);
        if (rows * cols > 0) {
            std::vector<int> flat(rows * cols);
            for (size_t i = 0; i < rows; ++i)
                for (size_t j = 0; j < cols; ++j)
                    flat[i * cols + j] = mat[i][j];
            ds.write(flat.data(), H5::PredType::NATIVE_INT);
        }
        return StatusOr<bool>::Ok(true);
    } catch (const H5::DataSetIException&) {
        return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::DATASET_WRITE_ERROR));
    } catch (const H5::Exception&) {
        return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::DATASET_WRITE_ERROR));
    }
}

StatusOr<bool> Hdf5Writer::writeDataset(const std::string& path, const std::vector<std::vector<double>>& mat) {
    if (!handle_.isValid()) return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::HANDLE_INVALID));
    { auto e = ensureParentsForDataset(path); if (!e.ok()) return e; }
    H5::H5File* fp = handle_.get();
    std::string p = normPath(path);
    try {
        const size_t rows = mat.size();
        if (rows == 0) {
            hsize_t dims[2] = {0, 0};
            H5::DataSpace sp(2, dims);
            fp->createDataSet(p, H5::PredType::NATIVE_DOUBLE, sp);
            return StatusOr<bool>::Ok(true);
        }
        const size_t cols = mat[0].size();
        for (size_t i = 1; i < rows; ++i)
            if (mat[i].size() != cols)
                return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::DATASET_WRITE_ERROR));
        hsize_t dims[2] = {rows, cols};
        H5::DataSpace sp(2, dims);
        H5::DataSet ds = fp->createDataSet(p, H5::PredType::NATIVE_DOUBLE, sp);
        if (rows * cols > 0) {
            std::vector<double> flat(rows * cols);
            for (size_t i = 0; i < rows; ++i)
                for (size_t j = 0; j < cols; ++j)
                    flat[i * cols + j] = mat[i][j];
            ds.write(flat.data(), H5::PredType::NATIVE_DOUBLE);
        }
        return StatusOr<bool>::Ok(true);
    } catch (const H5::DataSetIException&) {
        return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::DATASET_WRITE_ERROR));
    } catch (const H5::Exception&) {
        return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::DATASET_WRITE_ERROR));
    }
}

StatusOr<bool> Hdf5Writer::writeDataset(const std::string& path, const std::vector<std::vector<float>>& mat) {
    if (!handle_.isValid()) return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::HANDLE_INVALID));
    { auto e = ensureParentsForDataset(path); if (!e.ok()) return e; }
    H5::H5File* fp = handle_.get();
    std::string p = normPath(path);
    try {
        const size_t rows = mat.size();
        if (rows == 0) {
            hsize_t dims[2] = {0, 0};
            H5::DataSpace sp(2, dims);
            fp->createDataSet(p, H5::PredType::NATIVE_FLOAT, sp);
            return StatusOr<bool>::Ok(true);
        }
        const size_t cols = mat[0].size();
        for (size_t i = 1; i < rows; ++i)
            if (mat[i].size() != cols)
                return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::DATASET_WRITE_ERROR));
        hsize_t dims[2] = {rows, cols};
        H5::DataSpace sp(2, dims);
        H5::DataSet ds = fp->createDataSet(p, H5::PredType::NATIVE_FLOAT, sp);
        if (rows * cols > 0) {
            std::vector<float> flat(rows * cols);
            for (size_t i = 0; i < rows; ++i)
                for (size_t j = 0; j < cols; ++j)
                    flat[i * cols + j] = mat[i][j];
            ds.write(flat.data(), H5::PredType::NATIVE_FLOAT);
        }
        return StatusOr<bool>::Ok(true);
    } catch (const H5::DataSetIException&) {
        return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::DATASET_WRITE_ERROR));
    } catch (const H5::Exception&) {
        return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::DATASET_WRITE_ERROR));
    }
}

StatusOr<bool> Hdf5Writer::writeDataset(const std::string& path, const std::vector<std::vector<int>>& mat, const Hdf5WriteOptions& opts) {
    if (!handle_.isValid()) return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::HANDLE_INVALID));
    if (opts.chunkDims.empty() && opts.gzipLevel < 0) return writeDataset(path, mat);
    { auto e = ensureParentsForDataset(path); if (!e.ok()) return e; }
    H5::H5File* fp = handle_.get();
    std::string p = normPath(path);
    try {
        const size_t rows = mat.size();
        if (rows == 0) return writeDataset(path, mat);
        const size_t cols = mat[0].size();
        for (size_t i = 1; i < rows; ++i)
            if (mat[i].size() != cols)
                return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::DATASET_WRITE_ERROR));
        H5::DSetCreatPropList plist = makePlist2D(opts);
        hsize_t dims[2] = {rows, cols};
        H5::DataSpace sp(2, dims);
        H5::DataSet ds = fp->createDataSet(p, H5::PredType::NATIVE_INT, sp, plist);
        std::vector<int> flat(rows * cols);
        for (size_t i = 0; i < rows; ++i)
            for (size_t j = 0; j < cols; ++j)
                flat[i * cols + j] = mat[i][j];
        ds.write(flat.data(), H5::PredType::NATIVE_INT);
        return StatusOr<bool>::Ok(true);
    } catch (const std::invalid_argument&) {
        return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::COMPRESSION_ERROR));
    } catch (const H5::DataSetIException&) {
        return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::DATASET_WRITE_ERROR));
    } catch (const H5::Exception&) {
        return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::DATASET_WRITE_ERROR));
    }
}

StatusOr<bool> Hdf5Writer::writeDataset(const std::string& path, const std::vector<std::vector<double>>& mat, const Hdf5WriteOptions& opts) {
    if (!handle_.isValid()) return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::HANDLE_INVALID));
    if (opts.chunkDims.empty() && opts.gzipLevel < 0) return writeDataset(path, mat);
    { auto e = ensureParentsForDataset(path); if (!e.ok()) return e; }
    H5::H5File* fp = handle_.get();
    std::string p = normPath(path);
    try {
        const size_t rows = mat.size();
        if (rows == 0) return writeDataset(path, mat);
        const size_t cols = mat[0].size();
        for (size_t i = 1; i < rows; ++i)
            if (mat[i].size() != cols)
                return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::DATASET_WRITE_ERROR));
        H5::DSetCreatPropList plist = makePlist2D(opts);
        hsize_t dims[2] = {rows, cols};
        H5::DataSpace sp(2, dims);
        H5::DataSet ds = fp->createDataSet(p, H5::PredType::NATIVE_DOUBLE, sp, plist);
        std::vector<double> flat(rows * cols);
        for (size_t i = 0; i < rows; ++i)
            for (size_t j = 0; j < cols; ++j)
                flat[i * cols + j] = mat[i][j];
        ds.write(flat.data(), H5::PredType::NATIVE_DOUBLE);
        return StatusOr<bool>::Ok(true);
    } catch (const std::invalid_argument&) {
        return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::COMPRESSION_ERROR));
    } catch (const H5::DataSetIException&) {
        return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::DATASET_WRITE_ERROR));
    } catch (const H5::Exception&) {
        return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::DATASET_WRITE_ERROR));
    }
}

StatusOr<bool> Hdf5Writer::writeDataset(const std::string& path, const std::vector<std::vector<float>>& mat, const Hdf5WriteOptions& opts) {
    if (!handle_.isValid()) return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::HANDLE_INVALID));
    if (opts.chunkDims.empty() && opts.gzipLevel < 0) return writeDataset(path, mat);
    { auto e = ensureParentsForDataset(path); if (!e.ok()) return e; }
    H5::H5File* fp = handle_.get();
    std::string p = normPath(path);
    try {
        const size_t rows = mat.size();
        if (rows == 0) return writeDataset(path, mat);
        const size_t cols = mat[0].size();
        for (size_t i = 1; i < rows; ++i)
            if (mat[i].size() != cols)
                return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::DATASET_WRITE_ERROR));
        H5::DSetCreatPropList plist = makePlist2D(opts);
        hsize_t dims[2] = {rows, cols};
        H5::DataSpace sp(2, dims);
        H5::DataSet ds = fp->createDataSet(p, H5::PredType::NATIVE_FLOAT, sp, plist);
        std::vector<float> flat(rows * cols);
        for (size_t i = 0; i < rows; ++i)
            for (size_t j = 0; j < cols; ++j)
                flat[i * cols + j] = mat[i][j];
        ds.write(flat.data(), H5::PredType::NATIVE_FLOAT);
        return StatusOr<bool>::Ok(true);
    } catch (const std::invalid_argument&) {
        return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::COMPRESSION_ERROR));
    } catch (const H5::DataSetIException&) {
        return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::DATASET_WRITE_ERROR));
    } catch (const H5::Exception&) {
        return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::DATASET_WRITE_ERROR));
    }
}

StatusOr<bool> Hdf5Writer::writeDataset(const std::string& path, const std::vector<std::vector<std::vector<int>>>& cube) {
    if (!handle_.isValid()) return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::HANDLE_INVALID));
    { auto e = ensureParentsForDataset(path); if (!e.ok()) return e; }
    H5::H5File* fp = handle_.get();
    std::string p = normPath(path);
    try {
        const size_t d0 = cube.size();
        if (d0 == 0) {
            hsize_t dims[3] = {0, 0, 0};
            H5::DataSpace sp(3, dims);
            fp->createDataSet(p, H5::PredType::NATIVE_INT, sp);
            return StatusOr<bool>::Ok(true);
        }
        const size_t d1 = cube[0].size();
        const size_t d2 = d1 ? cube[0][0].size() : 0;
        for (size_t i = 0; i < d0; ++i) {
            if (cube[i].size() != d1) return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::DATASET_WRITE_ERROR));
            for (size_t j = 0; j < d1; ++j)
                if (cube[i][j].size() != d2) return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::DATASET_WRITE_ERROR));
        }
        hsize_t dims[3] = {d0, d1, d2};
        H5::DataSpace sp(3, dims);
        H5::DataSet ds = fp->createDataSet(p, H5::PredType::NATIVE_INT, sp);
        if (d0 * d1 * d2 > 0) {
            std::vector<int> flat(d0 * d1 * d2);
            for (size_t i = 0; i < d0; ++i)
                for (size_t j = 0; j < d1; ++j)
                    for (size_t k = 0; k < d2; ++k)
                        flat[i * (d1 * d2) + j * d2 + k] = cube[i][j][k];
            ds.write(flat.data(), H5::PredType::NATIVE_INT);
        }
        return StatusOr<bool>::Ok(true);
    } catch (const H5::DataSetIException&) {
        return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::DATASET_WRITE_ERROR));
    } catch (const H5::Exception&) {
        return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::DATASET_WRITE_ERROR));
    }
}

StatusOr<bool> Hdf5Writer::writeDataset(const std::string& path, const std::vector<std::vector<std::vector<double>>>& cube) {
    if (!handle_.isValid()) return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::HANDLE_INVALID));
    { auto e = ensureParentsForDataset(path); if (!e.ok()) return e; }
    H5::H5File* fp = handle_.get();
    std::string p = normPath(path);
    try {
        const size_t d0 = cube.size();
        if (d0 == 0) {
            hsize_t dims[3] = {0, 0, 0};
            H5::DataSpace sp(3, dims);
            fp->createDataSet(p, H5::PredType::NATIVE_DOUBLE, sp);
            return StatusOr<bool>::Ok(true);
        }
        const size_t d1 = cube[0].size();
        const size_t d2 = d1 ? cube[0][0].size() : 0;
        for (size_t i = 0; i < d0; ++i) {
            if (cube[i].size() != d1) return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::DATASET_WRITE_ERROR));
            for (size_t j = 0; j < d1; ++j)
                if (cube[i][j].size() != d2) return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::DATASET_WRITE_ERROR));
        }
        hsize_t dims[3] = {d0, d1, d2};
        H5::DataSpace sp(3, dims);
        H5::DataSet ds = fp->createDataSet(p, H5::PredType::NATIVE_DOUBLE, sp);
        if (d0 * d1 * d2 > 0) {
            std::vector<double> flat(d0 * d1 * d2);
            for (size_t i = 0; i < d0; ++i)
                for (size_t j = 0; j < d1; ++j)
                    for (size_t k = 0; k < d2; ++k)
                        flat[i * (d1 * d2) + j * d2 + k] = cube[i][j][k];
            ds.write(flat.data(), H5::PredType::NATIVE_DOUBLE);
        }
        return StatusOr<bool>::Ok(true);
    } catch (const H5::DataSetIException&) {
        return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::DATASET_WRITE_ERROR));
    } catch (const H5::Exception&) {
        return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::DATASET_WRITE_ERROR));
    }
}

StatusOr<bool> Hdf5Writer::writeDataset(const std::string& path, const std::vector<std::vector<std::vector<float>>>& cube) {
    if (!handle_.isValid()) return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::HANDLE_INVALID));
    { auto e = ensureParentsForDataset(path); if (!e.ok()) return e; }
    H5::H5File* fp = handle_.get();
    std::string p = normPath(path);
    try {
        const size_t d0 = cube.size();
        if (d0 == 0) {
            hsize_t dims[3] = {0, 0, 0};
            H5::DataSpace sp(3, dims);
            fp->createDataSet(p, H5::PredType::NATIVE_FLOAT, sp);
            return StatusOr<bool>::Ok(true);
        }
        const size_t d1 = cube[0].size();
        const size_t d2 = d1 ? cube[0][0].size() : 0;
        for (size_t i = 0; i < d0; ++i) {
            if (cube[i].size() != d1) return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::DATASET_WRITE_ERROR));
            for (size_t j = 0; j < d1; ++j)
                if (cube[i][j].size() != d2) return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::DATASET_WRITE_ERROR));
        }
        hsize_t dims[3] = {d0, d1, d2};
        H5::DataSpace sp(3, dims);
        H5::DataSet ds = fp->createDataSet(p, H5::PredType::NATIVE_FLOAT, sp);
        if (d0 * d1 * d2 > 0) {
            std::vector<float> flat(d0 * d1 * d2);
            for (size_t i = 0; i < d0; ++i)
                for (size_t j = 0; j < d1; ++j)
                    for (size_t k = 0; k < d2; ++k)
                        flat[i * (d1 * d2) + j * d2 + k] = cube[i][j][k];
            ds.write(flat.data(), H5::PredType::NATIVE_FLOAT);
        }
        return StatusOr<bool>::Ok(true);
    } catch (const H5::DataSetIException&) {
        return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::DATASET_WRITE_ERROR));
    } catch (const H5::Exception&) {
        return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::DATASET_WRITE_ERROR));
    }
}

StatusOr<bool> Hdf5Writer::writeDataset(const std::string& path, const std::vector<std::string>& vec) {
    if (!handle_.isValid()) return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::HANDLE_INVALID));
    { auto e = ensureParentsForDataset(path); if (!e.ok()) return e; }
    H5::H5File* fp = handle_.get();
    std::string p = normPath(path);
    try {
        const size_t n = vec.size();
        hsize_t dims[1] = {n};
        H5::DataSpace sp(1, dims);
        H5::StrType st(H5::PredType::C_S1, H5T_VARIABLE);
        H5::DataSet ds = fp->createDataSet(p, st, sp);
        if (n > 0) {
            std::vector<const char*> ptrs(n);
            for (size_t i = 0; i < n; ++i) ptrs[i] = vec[i].c_str();
            ds.write(ptrs.data(), st);
        }
        return StatusOr<bool>::Ok(true);
    } catch (const H5::DataSetIException&) {
        return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::DATASET_WRITE_ERROR));
    } catch (const H5::Exception&) {
        return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::DATASET_WRITE_ERROR));
    }
}

StatusOr<bool> Hdf5Writer::writeAttribute(const std::string& objectPath, const std::string& attrName, int value) {
    if (!handle_.isValid()) return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::HANDLE_INVALID));
    try {
        withObjForAttr(handle_.get(), objectPath, [&](H5::H5Object* obj) {
            H5::DataSpace sp(H5S_SCALAR);
            H5::Attribute a = obj->createAttribute(attrName, H5::PredType::NATIVE_INT, sp);
            a.write(H5::PredType::NATIVE_INT, &value);
        });
        return StatusOr<bool>::Ok(true);
    } catch (const H5::AttributeIException&) {
        return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::ATTRIBUTE_WRITE_ERROR));
    } catch (const H5::Exception&) {
        return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::ATTRIBUTE_WRITE_ERROR));
    }
}

StatusOr<bool> Hdf5Writer::writeAttribute(const std::string& objectPath, const std::string& attrName, double value) {
    std::string path = objectPath + "@" + attrName;
    Hdf5OpTrace trace("writeAttribute", path);
    if (!handle_.isValid()) {
        trace.fail("HANDLE_INVALID");
        return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::HANDLE_INVALID));
    }
    try {
        withObjForAttr(handle_.get(), objectPath, [&](H5::H5Object* obj) {
            H5::DataSpace sp(H5S_SCALAR);
            H5::Attribute a = obj->createAttribute(attrName, H5::PredType::NATIVE_DOUBLE, sp);
            a.write(H5::PredType::NATIVE_DOUBLE, &value);
        });
        return StatusOr<bool>::Ok(true);
    } catch (const H5::AttributeIException&) {
        trace.fail("ATTRIBUTE_WRITE_ERROR");
        return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::ATTRIBUTE_WRITE_ERROR));
    } catch (const H5::Exception&) {
        trace.fail("ATTRIBUTE_WRITE_ERROR");
        return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::ATTRIBUTE_WRITE_ERROR));
    }
}

StatusOr<bool> Hdf5Writer::writeAttribute(const std::string& objectPath, const std::string& attrName, float value) {
    if (!handle_.isValid()) return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::HANDLE_INVALID));
    try {
        withObjForAttr(handle_.get(), objectPath, [&](H5::H5Object* obj) {
            H5::DataSpace sp(H5S_SCALAR);
            H5::Attribute a = obj->createAttribute(attrName, H5::PredType::NATIVE_FLOAT, sp);
            a.write(H5::PredType::NATIVE_FLOAT, &value);
        });
        return StatusOr<bool>::Ok(true);
    } catch (const H5::AttributeIException&) {
        return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::ATTRIBUTE_WRITE_ERROR));
    } catch (const H5::Exception&) {
        return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::ATTRIBUTE_WRITE_ERROR));
    }
}

StatusOr<bool> Hdf5Writer::writeAttribute(const std::string& objectPath, const std::string& attrName, const std::string& value) {
    if (!handle_.isValid()) return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::HANDLE_INVALID));
    try {
        withObjForAttr(handle_.get(), objectPath, [&](H5::H5Object* obj) {
            H5::StrType st(H5::PredType::C_S1, H5T_VARIABLE);
            H5::DataSpace sp(H5S_SCALAR);
            H5::Attribute a = obj->createAttribute(attrName, st, sp);
            H5std_string s(value);
            a.write(st, s);
        });
        return StatusOr<bool>::Ok(true);
    } catch (const H5::AttributeIException&) {
        return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::ATTRIBUTE_WRITE_ERROR));
    } catch (const H5::Exception&) {
        return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::ATTRIBUTE_WRITE_ERROR));
    }
}

StatusOr<bool> Hdf5Writer::writeAttributes(const std::string& objectPath, const std::map<std::string, int>& attrs) {
    for (const auto& kv : attrs) {
        auto r = writeAttribute(objectPath, kv.first, kv.second);
        if (!r.ok()) return r;
    }
    return StatusOr<bool>::Ok(true);
}

StatusOr<bool> Hdf5Writer::writeAttributes(const std::string& objectPath, const std::map<std::string, double>& attrs) {
    for (const auto& kv : attrs) {
        auto r = writeAttribute(objectPath, kv.first, kv.second);
        if (!r.ok()) return r;
    }
    return StatusOr<bool>::Ok(true);
}

StatusOr<bool> Hdf5Writer::writeAttributes(const std::string& objectPath, const std::map<std::string, std::string>& attrs) {
    for (const auto& kv : attrs) {
        auto r = writeAttribute(objectPath, kv.first, kv.second);
        if (!r.ok()) return r;
    }
    return StatusOr<bool>::Ok(true);
}

StatusOr<bool> Hdf5Writer::writeDatasetsBatch(const std::map<std::string, std::vector<double>>& pathToData) {
    if (!handle_.isValid()) return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::HANDLE_INVALID));
    for (const auto& kv : pathToData) {
        auto r = writeDataset(kv.first, kv.second);
        if (!r.ok()) return r;
    }
    return StatusOr<bool>::Ok(true);
}

StatusOr<bool> Hdf5Writer::deleteNode(const std::string& path) {
    if (!handle_.isValid()) return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::HANDLE_INVALID));
    H5::H5File* fp = handle_.get();
    std::string p = normPath(path);
    if (p.empty() || p == "/") return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::PATH_NOT_FOUND));
    std::string link = linkName(path);
    std::string parent = parentPath(path);
    try {
        if (parent.empty()) {
            fp->unlink(link);
        } else {
            H5::Group g = fp->openGroup(parent);
            g.unlink(link);
        }
        return StatusOr<bool>::Ok(true);
    } catch (const H5::Exception&) {
        return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::PATH_NOT_FOUND));
    }
}

StatusOr<bool> Hdf5Writer::deleteNodes(const std::vector<std::string>& paths) {
    for (const auto& path : paths) {
        auto r = deleteNode(path);
        if (!r.ok()) return r;
    }
    return StatusOr<bool>::Ok(true);
}

StatusOr<bool> Hdf5Writer::appendDataset(const std::string& path, const std::vector<double>& data) {
    if (!handle_.isValid()) return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::HANDLE_INVALID));
    { auto e = ensureParentsForDataset(path); if (!e.ok()) return e; }
    H5::H5File* fp = handle_.get();
    std::string p = normPath(path);
    const hsize_t n = static_cast<hsize_t>(data.size());
    if (n == 0) return StatusOr<bool>::Ok(true);
    try {
        if (!datasetExistsAt(fp, p)) {
            hsize_t dims[1] = { n };
            hsize_t maxdims[1] = { H5S_UNLIMITED };
            H5::DataSpace sp(1, dims, maxdims);
            hsize_t chunk[1] = { (n < 4096) ? (n ? n : 1) : 4096 };
            H5::DSetCreatPropList plist;
            plist.setChunk(1, chunk);
            H5::DataSet ds = fp->createDataSet(p, H5::PredType::NATIVE_DOUBLE, sp, plist);
            if (n > 0) ds.write(data.data(), H5::PredType::NATIVE_DOUBLE);
            return StatusOr<bool>::Ok(true);
        }
        H5::DataSet ds = fp->openDataSet(p);
        H5::DataSpace fspace = ds.getSpace();
        int ndims = fspace.getSimpleExtentNdims();
        if (ndims != 1) return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::DATASET_WRITE_ERROR));
        hsize_t cur[1], max[1];
        fspace.getSimpleExtentDims(cur, max);
        hsize_t newSize = cur[0] + n;
        ds.extend(&newSize);
        H5::DataSpace fileSpace = ds.getSpace();
        hsize_t start[1] = { cur[0] };
        hsize_t count[1] = { n };
        fileSpace.selectHyperslab(H5S_SELECT_SET, count, start);
        H5::DataSpace memSpace(1, count);
        ds.write(data.data(), H5::PredType::NATIVE_DOUBLE, memSpace, fileSpace);
        return StatusOr<bool>::Ok(true);
    } catch (const H5::DataSetIException&) {
        return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::DATASET_WRITE_ERROR));
    } catch (const H5::Exception&) {
        return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::DATASET_WRITE_ERROR));
    }
}

} // namespace hdf5
} // namespace io
} // namespace common
} // namespace AIstudy


#endif // AISTUDY_HDF5_ENABLED
