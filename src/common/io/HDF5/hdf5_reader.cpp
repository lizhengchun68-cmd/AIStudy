#ifdef AISTUDY_HDF5_ENABLED

#include "common/io/hdf5/hdf5_reader.h"
#include "common/common_status/exception/error_category.h"
#include "common/common_status/status_or.h"
#include "common/io/hdf5/hdf5_handles.h"
#include "common/io/hdf5/hdf5_logging.h"
#include <H5Cpp.h>
#include <H5Exception.h>
#include <memory>

namespace AIstudy {
namespace common {
namespace io {
namespace hdf5 {


namespace {

ErrorCodeWrapper makeHdf5Err(Hdf5Error e) {
    return ErrorCodeWrapper(static_cast<int>(e), ErrorCategory::HDF5);
}

std::string normPath(const std::string& path) {
    if (path.empty() || path[0] == '/') return path;
    return "/" + path;
}

H5::Attribute openAttrFromPath(const H5::H5File* file, const std::string& path, const std::string& attrName) {
    std::string p = normPath(path);
    if (p.empty() || p == "/") return file->openAttribute(attrName);
    try {
        H5::DataSet d = file->openDataSet(p);
        return d.openAttribute(attrName);
    } catch (...) {}
    H5::Group g = file->openGroup(p);
    return g.openAttribute(attrName);
}

void collectAttrNamesOp(H5::H5Object&, H5std_string name, void* op) {
    auto* v = static_cast<std::vector<std::string>*>(op);
    v->push_back(std::move(name));
}

} // namespace

Hdf5Reader::~Hdf5Reader() { closeFile(); }

void Hdf5Reader::closeFile() {
    std::string p = filepath_.empty() ? "(none)" : filepath_;
    Hdf5OpTrace trace("closeFile", p);
    handle_ = H5FileHandle();
    filepath_.clear();
}

bool Hdf5Reader::isFileOpen() const { return handle_.isValid(); }

const std::string& Hdf5Reader::getFilePath() const { return filepath_; }

StatusOr<bool> Hdf5Reader::openFile(const std::string& filepath) {
    Hdf5OpTrace trace("openFile", filepath);
    closeFile();
    try {
        H5::Exception::dontPrint();
        auto p = std::unique_ptr<H5::H5File>(new H5::H5File(filepath, H5F_ACC_RDONLY));
        handle_ = H5FileHandle(std::move(p));
        filepath_ = filepath;
        return StatusOr<bool>::Ok(true);
    } catch (const H5::FileIException&) {
        trace.fail("FILE_OPEN_ERROR");
        return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::FILE_OPEN_ERROR));
    } catch (const H5::Exception&) {
        trace.fail("FILE_OPEN_ERROR");
        return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::FILE_OPEN_ERROR));
    }
}

bool Hdf5Reader::datasetExists(const std::string& path) const {
    const H5::H5File* fp = handle_.get();
    if (!fp) return false;
    try {
        std::string p = normPath(path);
        H5::DataSet ds = fp->openDataSet(p);
        ds.close();
        return true;
    } catch (...) { return false; }
}

bool Hdf5Reader::groupExists(const std::string& path) const {
    const H5::H5File* fp = handle_.get();
    if (!fp) return false;
    if (path.empty() || path == "/") return true;
    try {
        std::string p = normPath(path);
        H5::Group g = fp->openGroup(p);
        g.close();
        return true;
    } catch (...) { return false; }
}

bool Hdf5Reader::attributeExists(const std::string& objectPath, const std::string& attrName) const {
    const H5::H5File* fp = handle_.get();
    if (!fp) return false;
    try {
        std::string p = normPath(objectPath);
        if (p.empty() || p == "/") {
            H5::Attribute a = fp->openAttribute(attrName);
            a.close();
            return true;
        }
        try {
            H5::DataSet ds = fp->openDataSet(p);
            H5::Attribute a = ds.openAttribute(attrName);
            a.close();
            return true;
        } catch (...) {}
        H5::Group g = fp->openGroup(p);
        H5::Attribute a = g.openAttribute(attrName);
        a.close();
        return true;
    } catch (...) { return false; }
}

StatusOr<std::vector<hsize_t>> Hdf5Reader::getDatasetShape(const std::string& path) const {
    if (!handle_.isValid()) return StatusOr<std::vector<hsize_t>>::Fail(makeHdf5Err(Hdf5Error::HANDLE_INVALID));
    const H5::H5File* fp = handle_.get();
    try {
        std::string p = normPath(path);
        H5::DataSet ds = fp->openDataSet(p);
        H5::DataSpace sp = ds.getSpace();
        int ndims = sp.getSimpleExtentNdims();
        std::vector<hsize_t> dims(static_cast<size_t>(ndims), 0);
        if (ndims > 0) sp.getSimpleExtentDims(dims.data(), nullptr);
        return StatusOr<std::vector<hsize_t>>::Ok(dims);
    } catch (const H5::DataSetIException&) {
        return StatusOr<std::vector<hsize_t>>::Fail(makeHdf5Err(Hdf5Error::DATASET_READ_ERROR));
    } catch (const H5::Exception&) {
        return StatusOr<std::vector<hsize_t>>::Fail(makeHdf5Err(Hdf5Error::DATASET_READ_ERROR));
    }
}

// --- readDataset ---

template <>
StatusOr<int> Hdf5Reader::readDataset<int>(const std::string& path) const {
    if (!handle_.isValid()) return StatusOr<int>::Fail(makeHdf5Err(Hdf5Error::HANDLE_INVALID));
    const H5::H5File* fp = handle_.get();
    try {
        std::string p = normPath(path);
        H5::DataSet ds = fp->openDataSet(p);
        int v = 0;
        ds.read(&v, H5::PredType::NATIVE_INT);
        return StatusOr<int>::Ok(v);
    } catch (const H5::DataSetIException&) {
        return StatusOr<int>::Fail(makeHdf5Err(Hdf5Error::DATASET_READ_ERROR));
    } catch (const H5::Exception&) {
        return StatusOr<int>::Fail(makeHdf5Err(Hdf5Error::DATASET_READ_ERROR));
    }
}

template <>
StatusOr<double> Hdf5Reader::readDataset<double>(const std::string& path) const {
    if (!handle_.isValid()) return StatusOr<double>::Fail(makeHdf5Err(Hdf5Error::HANDLE_INVALID));
    const H5::H5File* fp = handle_.get();
    try {
        std::string p = normPath(path);
        H5::DataSet ds = fp->openDataSet(p);
        double v = 0.0;
        ds.read(&v, H5::PredType::NATIVE_DOUBLE);
        return StatusOr<double>::Ok(v);
    } catch (const H5::DataSetIException&) {
        return StatusOr<double>::Fail(makeHdf5Err(Hdf5Error::DATASET_READ_ERROR));
    } catch (const H5::Exception&) {
        return StatusOr<double>::Fail(makeHdf5Err(Hdf5Error::DATASET_READ_ERROR));
    }
}

template <>
StatusOr<float> Hdf5Reader::readDataset<float>(const std::string& path) const {
    if (!handle_.isValid()) return StatusOr<float>::Fail(makeHdf5Err(Hdf5Error::HANDLE_INVALID));
    const H5::H5File* fp = handle_.get();
    try {
        std::string p = normPath(path);
        H5::DataSet ds = fp->openDataSet(p);
        float v = 0.f;
        ds.read(&v, H5::PredType::NATIVE_FLOAT);
        return StatusOr<float>::Ok(v);
    } catch (const H5::DataSetIException&) {
        return StatusOr<float>::Fail(makeHdf5Err(Hdf5Error::DATASET_READ_ERROR));
    } catch (const H5::Exception&) {
        return StatusOr<float>::Fail(makeHdf5Err(Hdf5Error::DATASET_READ_ERROR));
    }
}

template <>
StatusOr<std::string> Hdf5Reader::readDataset<std::string>(const std::string& path) const {
    if (!handle_.isValid()) return StatusOr<std::string>::Fail(makeHdf5Err(Hdf5Error::HANDLE_INVALID));
    const H5::H5File* fp = handle_.get();
    try {
        std::string p = normPath(path);
        H5::DataSet ds = fp->openDataSet(p);
        H5::DataType dt = ds.getDataType();
        H5std_string s;
        ds.read(s, dt);
        return StatusOr<std::string>::Ok(std::string(s.c_str()));
    } catch (const H5::DataSetIException&) {
        return StatusOr<std::string>::Fail(makeHdf5Err(Hdf5Error::DATASET_READ_ERROR));
    } catch (const H5::Exception&) {
        return StatusOr<std::string>::Fail(makeHdf5Err(Hdf5Error::DATASET_READ_ERROR));
    }
}

template <>
StatusOr<std::vector<int>> Hdf5Reader::readDataset<std::vector<int>>(const std::string& path) const {
    if (!handle_.isValid()) return StatusOr<std::vector<int>>::Fail(makeHdf5Err(Hdf5Error::HANDLE_INVALID));
    const H5::H5File* fp = handle_.get();
    try {
        std::string p = normPath(path);
        H5::DataSet ds = fp->openDataSet(p);
        H5::DataSpace sp = ds.getSpace();
        int nd = sp.getSimpleExtentNdims();
        if (nd != 1) return StatusOr<std::vector<int>>::Fail(makeHdf5Err(Hdf5Error::DATASET_READ_ERROR));
        hsize_t dims[1] = {0};
        sp.getSimpleExtentDims(dims);
        std::vector<int> out(static_cast<size_t>(dims[0]));
        if (!out.empty())
            ds.read(out.data(), H5::PredType::NATIVE_INT);
        return StatusOr<std::vector<int>>::Ok(out);
    } catch (const H5::DataSetIException&) {
        return StatusOr<std::vector<int>>::Fail(makeHdf5Err(Hdf5Error::DATASET_READ_ERROR));
    } catch (const H5::Exception&) {
        return StatusOr<std::vector<int>>::Fail(makeHdf5Err(Hdf5Error::DATASET_READ_ERROR));
    }
}

template <>
StatusOr<std::vector<double>> Hdf5Reader::readDataset<std::vector<double>>(const std::string& path) const {
    Hdf5OpTrace trace("readDataset", path);
    if (!handle_.isValid()) {
        trace.fail("HANDLE_INVALID");
        return StatusOr<std::vector<double>>::Fail(makeHdf5Err(Hdf5Error::HANDLE_INVALID));
    }
    const H5::H5File* fp = handle_.get();
    try {
        std::string p = normPath(path);
        H5::DataSet ds = fp->openDataSet(p);
        H5::DataSpace sp = ds.getSpace();
        int nd = sp.getSimpleExtentNdims();
        if (nd != 1) {
            trace.fail("DATASET_READ_ERROR");
            return StatusOr<std::vector<double>>::Fail(makeHdf5Err(Hdf5Error::DATASET_READ_ERROR));
        }
        hsize_t dims[1] = {0};
        sp.getSimpleExtentDims(dims);
        std::vector<double> out(static_cast<size_t>(dims[0]));
        if (!out.empty())
            ds.read(out.data(), H5::PredType::NATIVE_DOUBLE);
        trace.setDetail("len=" + std::to_string(out.size()));
        return StatusOr<std::vector<double>>::Ok(out);
    } catch (const H5::DataSetIException&) {
        trace.fail("DATASET_READ_ERROR");
        return StatusOr<std::vector<double>>::Fail(makeHdf5Err(Hdf5Error::DATASET_READ_ERROR));
    } catch (const H5::Exception&) {
        trace.fail("DATASET_READ_ERROR");
        return StatusOr<std::vector<double>>::Fail(makeHdf5Err(Hdf5Error::DATASET_READ_ERROR));
    }
}

template <>
StatusOr<std::vector<float>> Hdf5Reader::readDataset<std::vector<float>>(const std::string& path) const {
    if (!handle_.isValid()) return StatusOr<std::vector<float>>::Fail(makeHdf5Err(Hdf5Error::HANDLE_INVALID));
    const H5::H5File* fp = handle_.get();
    try {
        std::string p = normPath(path);
        H5::DataSet ds = fp->openDataSet(p);
        H5::DataSpace sp = ds.getSpace();
        int nd = sp.getSimpleExtentNdims();
        if (nd != 1) return StatusOr<std::vector<float>>::Fail(makeHdf5Err(Hdf5Error::DATASET_READ_ERROR));
        hsize_t dims[1] = {0};
        sp.getSimpleExtentDims(dims);
        std::vector<float> out(static_cast<size_t>(dims[0]));
        if (!out.empty())
            ds.read(out.data(), H5::PredType::NATIVE_FLOAT);
        return StatusOr<std::vector<float>>::Ok(out);
    } catch (const H5::DataSetIException&) {
        return StatusOr<std::vector<float>>::Fail(makeHdf5Err(Hdf5Error::DATASET_READ_ERROR));
    } catch (const H5::Exception&) {
        return StatusOr<std::vector<float>>::Fail(makeHdf5Err(Hdf5Error::DATASET_READ_ERROR));
    }
}

template <>
StatusOr<std::vector<std::vector<int>>> Hdf5Reader::readDataset<std::vector<std::vector<int>>>(const std::string& path) const {
    if (!handle_.isValid()) return StatusOr<std::vector<std::vector<int>>>::Fail(makeHdf5Err(Hdf5Error::HANDLE_INVALID));
    const H5::H5File* fp = handle_.get();
    try {
        std::string p = normPath(path);
        H5::DataSet ds = fp->openDataSet(p);
        H5::DataSpace sp = ds.getSpace();
        int nd = sp.getSimpleExtentNdims();
        if (nd != 2) return StatusOr<std::vector<std::vector<int>>>::Fail(makeHdf5Err(Hdf5Error::DATASET_READ_ERROR));
        hsize_t dims[2] = {0, 0};
        sp.getSimpleExtentDims(dims);
        const hsize_t rows = dims[0], cols = dims[1];
        std::vector<std::vector<int>> out(static_cast<size_t>(rows), std::vector<int>(static_cast<size_t>(cols)));
        if (rows * cols > 0) {
            std::vector<int> flat(static_cast<size_t>(rows * cols));
            ds.read(flat.data(), H5::PredType::NATIVE_INT);
            for (hsize_t i = 0; i < rows; ++i)
                for (hsize_t j = 0; j < cols; ++j)
                    out[static_cast<size_t>(i)][static_cast<size_t>(j)] = flat[static_cast<size_t>(i * cols + j)];
        }
        return StatusOr<std::vector<std::vector<int>>>::Ok(out);
    } catch (const H5::DataSetIException&) {
        return StatusOr<std::vector<std::vector<int>>>::Fail(makeHdf5Err(Hdf5Error::DATASET_READ_ERROR));
    } catch (const H5::Exception&) {
        return StatusOr<std::vector<std::vector<int>>>::Fail(makeHdf5Err(Hdf5Error::DATASET_READ_ERROR));
    }
}

template <>
StatusOr<std::vector<std::vector<double>>> Hdf5Reader::readDataset<std::vector<std::vector<double>>>(const std::string& path) const {
    if (!handle_.isValid()) return StatusOr<std::vector<std::vector<double>>>::Fail(makeHdf5Err(Hdf5Error::HANDLE_INVALID));
    const H5::H5File* fp = handle_.get();
    try {
        std::string p = normPath(path);
        H5::DataSet ds = fp->openDataSet(p);
        H5::DataSpace sp = ds.getSpace();
        int nd = sp.getSimpleExtentNdims();
        if (nd != 2) return StatusOr<std::vector<std::vector<double>>>::Fail(makeHdf5Err(Hdf5Error::DATASET_READ_ERROR));
        hsize_t dims[2] = {0, 0};
        sp.getSimpleExtentDims(dims);
        const hsize_t rows = dims[0], cols = dims[1];
        std::vector<std::vector<double>> out(static_cast<size_t>(rows), std::vector<double>(static_cast<size_t>(cols)));
        if (rows * cols > 0) {
            std::vector<double> flat(static_cast<size_t>(rows * cols));
            ds.read(flat.data(), H5::PredType::NATIVE_DOUBLE);
            for (hsize_t i = 0; i < rows; ++i)
                for (hsize_t j = 0; j < cols; ++j)
                    out[static_cast<size_t>(i)][static_cast<size_t>(j)] = flat[static_cast<size_t>(i * cols + j)];
        }
        return StatusOr<std::vector<std::vector<double>>>::Ok(out);
    } catch (const H5::DataSetIException&) {
        return StatusOr<std::vector<std::vector<double>>>::Fail(makeHdf5Err(Hdf5Error::DATASET_READ_ERROR));
    } catch (const H5::Exception&) {
        return StatusOr<std::vector<std::vector<double>>>::Fail(makeHdf5Err(Hdf5Error::DATASET_READ_ERROR));
    }
}

template <>
StatusOr<std::vector<std::vector<float>>> Hdf5Reader::readDataset<std::vector<std::vector<float>>>(const std::string& path) const {
    if (!handle_.isValid()) return StatusOr<std::vector<std::vector<float>>>::Fail(makeHdf5Err(Hdf5Error::HANDLE_INVALID));
    const H5::H5File* fp = handle_.get();
    try {
        std::string p = normPath(path);
        H5::DataSet ds = fp->openDataSet(p);
        H5::DataSpace sp = ds.getSpace();
        int nd = sp.getSimpleExtentNdims();
        if (nd != 2) return StatusOr<std::vector<std::vector<float>>>::Fail(makeHdf5Err(Hdf5Error::DATASET_READ_ERROR));
        hsize_t dims[2] = {0, 0};
        sp.getSimpleExtentDims(dims);
        const hsize_t rows = dims[0], cols = dims[1];
        std::vector<std::vector<float>> out(static_cast<size_t>(rows), std::vector<float>(static_cast<size_t>(cols)));
        if (rows * cols > 0) {
            std::vector<float> flat(static_cast<size_t>(rows * cols));
            ds.read(flat.data(), H5::PredType::NATIVE_FLOAT);
            for (hsize_t i = 0; i < rows; ++i)
                for (hsize_t j = 0; j < cols; ++j)
                    out[static_cast<size_t>(i)][static_cast<size_t>(j)] = flat[static_cast<size_t>(i * cols + j)];
        }
        return StatusOr<std::vector<std::vector<float>>>::Ok(out);
    } catch (const H5::DataSetIException&) {
        return StatusOr<std::vector<std::vector<float>>>::Fail(makeHdf5Err(Hdf5Error::DATASET_READ_ERROR));
    } catch (const H5::Exception&) {
        return StatusOr<std::vector<std::vector<float>>>::Fail(makeHdf5Err(Hdf5Error::DATASET_READ_ERROR));
    }
}

template <>
StatusOr<std::vector<std::vector<std::vector<int>>>> Hdf5Reader::readDataset<std::vector<std::vector<std::vector<int>>>>(const std::string& path) const {
    if (!handle_.isValid()) return StatusOr<std::vector<std::vector<std::vector<int>>>>::Fail(makeHdf5Err(Hdf5Error::HANDLE_INVALID));
    const H5::H5File* fp = handle_.get();
    try {
        std::string p = normPath(path);
        H5::DataSet ds = fp->openDataSet(p);
        H5::DataSpace sp = ds.getSpace();
        int nd = sp.getSimpleExtentNdims();
        if (nd != 3) return StatusOr<std::vector<std::vector<std::vector<int>>>>::Fail(makeHdf5Err(Hdf5Error::DATASET_READ_ERROR));
        hsize_t dims[3] = {0, 0, 0};
        sp.getSimpleExtentDims(dims);
        const hsize_t d0 = dims[0], d1 = dims[1], d2 = dims[2];
        std::vector<std::vector<std::vector<int>>> out(
            static_cast<size_t>(d0),
            std::vector<std::vector<int>>(static_cast<size_t>(d1), std::vector<int>(static_cast<size_t>(d2))));
        if (d0 * d1 * d2 > 0) {
            std::vector<int> flat(static_cast<size_t>(d0 * d1 * d2));
            ds.read(flat.data(), H5::PredType::NATIVE_INT);
            for (hsize_t i = 0; i < d0; ++i)
                for (hsize_t j = 0; j < d1; ++j)
                    for (hsize_t k = 0; k < d2; ++k)
                        out[static_cast<size_t>(i)][static_cast<size_t>(j)][static_cast<size_t>(k)] =
                            flat[static_cast<size_t>(i * (d1 * d2) + j * d2 + k)];
        }
        return StatusOr<std::vector<std::vector<std::vector<int>>>>::Ok(out);
    } catch (const H5::DataSetIException&) {
        return StatusOr<std::vector<std::vector<std::vector<int>>>>::Fail(makeHdf5Err(Hdf5Error::DATASET_READ_ERROR));
    } catch (const H5::Exception&) {
        return StatusOr<std::vector<std::vector<std::vector<int>>>>::Fail(makeHdf5Err(Hdf5Error::DATASET_READ_ERROR));
    }
}

template <>
StatusOr<std::vector<std::vector<std::vector<double>>>> Hdf5Reader::readDataset<std::vector<std::vector<std::vector<double>>>>(const std::string& path) const {
    if (!handle_.isValid()) return StatusOr<std::vector<std::vector<std::vector<double>>>>::Fail(makeHdf5Err(Hdf5Error::HANDLE_INVALID));
    const H5::H5File* fp = handle_.get();
    try {
        std::string p = normPath(path);
        H5::DataSet ds = fp->openDataSet(p);
        H5::DataSpace sp = ds.getSpace();
        int nd = sp.getSimpleExtentNdims();
        if (nd != 3) return StatusOr<std::vector<std::vector<std::vector<double>>>>::Fail(makeHdf5Err(Hdf5Error::DATASET_READ_ERROR));
        hsize_t dims[3] = {0, 0, 0};
        sp.getSimpleExtentDims(dims);
        const hsize_t d0 = dims[0], d1 = dims[1], d2 = dims[2];
        std::vector<std::vector<std::vector<double>>> out(
            static_cast<size_t>(d0),
            std::vector<std::vector<double>>(static_cast<size_t>(d1), std::vector<double>(static_cast<size_t>(d2))));
        if (d0 * d1 * d2 > 0) {
            std::vector<double> flat(static_cast<size_t>(d0 * d1 * d2));
            ds.read(flat.data(), H5::PredType::NATIVE_DOUBLE);
            for (hsize_t i = 0; i < d0; ++i)
                for (hsize_t j = 0; j < d1; ++j)
                    for (hsize_t k = 0; k < d2; ++k)
                        out[static_cast<size_t>(i)][static_cast<size_t>(j)][static_cast<size_t>(k)] =
                            flat[static_cast<size_t>(i * (d1 * d2) + j * d2 + k)];
        }
        return StatusOr<std::vector<std::vector<std::vector<double>>>>::Ok(out);
    } catch (const H5::DataSetIException&) {
        return StatusOr<std::vector<std::vector<std::vector<double>>>>::Fail(makeHdf5Err(Hdf5Error::DATASET_READ_ERROR));
    } catch (const H5::Exception&) {
        return StatusOr<std::vector<std::vector<std::vector<double>>>>::Fail(makeHdf5Err(Hdf5Error::DATASET_READ_ERROR));
    }
}

template <>
StatusOr<std::vector<std::vector<std::vector<float>>>> Hdf5Reader::readDataset<std::vector<std::vector<std::vector<float>>>>(const std::string& path) const {
    if (!handle_.isValid()) return StatusOr<std::vector<std::vector<std::vector<float>>>>::Fail(makeHdf5Err(Hdf5Error::HANDLE_INVALID));
    const H5::H5File* fp = handle_.get();
    try {
        std::string p = normPath(path);
        H5::DataSet ds = fp->openDataSet(p);
        H5::DataSpace sp = ds.getSpace();
        int nd = sp.getSimpleExtentNdims();
        if (nd != 3) return StatusOr<std::vector<std::vector<std::vector<float>>>>::Fail(makeHdf5Err(Hdf5Error::DATASET_READ_ERROR));
        hsize_t dims[3] = {0, 0, 0};
        sp.getSimpleExtentDims(dims);
        const hsize_t d0 = dims[0], d1 = dims[1], d2 = dims[2];
        std::vector<std::vector<std::vector<float>>> out(
            static_cast<size_t>(d0),
            std::vector<std::vector<float>>(static_cast<size_t>(d1), std::vector<float>(static_cast<size_t>(d2))));
        if (d0 * d1 * d2 > 0) {
            std::vector<float> flat(static_cast<size_t>(d0 * d1 * d2));
            ds.read(flat.data(), H5::PredType::NATIVE_FLOAT);
            for (hsize_t i = 0; i < d0; ++i)
                for (hsize_t j = 0; j < d1; ++j)
                    for (hsize_t k = 0; k < d2; ++k)
                        out[static_cast<size_t>(i)][static_cast<size_t>(j)][static_cast<size_t>(k)] =
                            flat[static_cast<size_t>(i * (d1 * d2) + j * d2 + k)];
        }
        return StatusOr<std::vector<std::vector<std::vector<float>>>>::Ok(out);
    } catch (const H5::DataSetIException&) {
        return StatusOr<std::vector<std::vector<std::vector<float>>>>::Fail(makeHdf5Err(Hdf5Error::DATASET_READ_ERROR));
    } catch (const H5::Exception&) {
        return StatusOr<std::vector<std::vector<std::vector<float>>>>::Fail(makeHdf5Err(Hdf5Error::DATASET_READ_ERROR));
    }
}

template <>
StatusOr<std::vector<std::string>> Hdf5Reader::readDataset<std::vector<std::string>>(const std::string& path) const {
    if (!handle_.isValid()) return StatusOr<std::vector<std::string>>::Fail(makeHdf5Err(Hdf5Error::HANDLE_INVALID));
    const H5::H5File* fp = handle_.get();
    try {
        std::string p = normPath(path);
        H5::DataSet ds = fp->openDataSet(p);
        H5::DataSpace fsp = ds.getSpace();
        int nd = fsp.getSimpleExtentNdims();
        if (nd != 1) return StatusOr<std::vector<std::string>>::Fail(makeHdf5Err(Hdf5Error::DATASET_READ_ERROR));
        hsize_t dims[1] = {0};
        fsp.getSimpleExtentDims(dims);
        const hsize_t n = dims[0];
        std::vector<std::string> out(static_cast<size_t>(n));
        if (n == 0) return StatusOr<std::vector<std::string>>::Ok(out);
        H5::StrType st(H5::PredType::C_S1, H5T_VARIABLE);
        for (hsize_t i = 0; i < n; ++i) {
            H5::DataSpace msp(H5S_SCALAR);
            H5::DataSpace fsel = ds.getSpace();
            hsize_t start[1] = {i}, count[1] = {1};
            fsel.selectHyperslab(H5S_SELECT_SET, count, start);
            H5std_string s;
            ds.read(s, st, msp, fsel);
            out[static_cast<size_t>(i)] = std::string(s.c_str());
        }
        return StatusOr<std::vector<std::string>>::Ok(out);
    } catch (const H5::DataSetIException&) {
        return StatusOr<std::vector<std::string>>::Fail(makeHdf5Err(Hdf5Error::DATASET_READ_ERROR));
    } catch (const H5::Exception&) {
        return StatusOr<std::vector<std::string>>::Fail(makeHdf5Err(Hdf5Error::DATASET_READ_ERROR));
    }
}

// --- readAttribute ---

template <>
StatusOr<int> Hdf5Reader::readAttribute<int>(const std::string& objectPath, const std::string& attrName) const {
    if (!handle_.isValid()) return StatusOr<int>::Fail(makeHdf5Err(Hdf5Error::HANDLE_INVALID));
    try {
        H5::Attribute a = openAttrFromPath(handle_.get(), objectPath, attrName);
        int v = 0;
        a.read(H5::PredType::NATIVE_INT, &v);
        return StatusOr<int>::Ok(v);
    } catch (const H5::AttributeIException&) {
        return StatusOr<int>::Fail(makeHdf5Err(Hdf5Error::ATTRIBUTE_READ_ERROR));
    } catch (const H5::Exception&) {
        return StatusOr<int>::Fail(makeHdf5Err(Hdf5Error::ATTRIBUTE_READ_ERROR));
    }
}

template <>
StatusOr<double> Hdf5Reader::readAttribute<double>(const std::string& objectPath, const std::string& attrName) const {
    if (!handle_.isValid()) return StatusOr<double>::Fail(makeHdf5Err(Hdf5Error::HANDLE_INVALID));
    try {
        H5::Attribute a = openAttrFromPath(handle_.get(), objectPath, attrName);
        double v = 0.0;
        a.read(H5::PredType::NATIVE_DOUBLE, &v);
        return StatusOr<double>::Ok(v);
    } catch (const H5::AttributeIException&) {
        return StatusOr<double>::Fail(makeHdf5Err(Hdf5Error::ATTRIBUTE_READ_ERROR));
    } catch (const H5::Exception&) {
        return StatusOr<double>::Fail(makeHdf5Err(Hdf5Error::ATTRIBUTE_READ_ERROR));
    }
}

template <>
StatusOr<float> Hdf5Reader::readAttribute<float>(const std::string& objectPath, const std::string& attrName) const {
    if (!handle_.isValid()) return StatusOr<float>::Fail(makeHdf5Err(Hdf5Error::HANDLE_INVALID));
    try {
        H5::Attribute a = openAttrFromPath(handle_.get(), objectPath, attrName);
        float v = 0.f;
        a.read(H5::PredType::NATIVE_FLOAT, &v);
        return StatusOr<float>::Ok(v);
    } catch (const H5::AttributeIException&) {
        return StatusOr<float>::Fail(makeHdf5Err(Hdf5Error::ATTRIBUTE_READ_ERROR));
    } catch (const H5::Exception&) {
        return StatusOr<float>::Fail(makeHdf5Err(Hdf5Error::ATTRIBUTE_READ_ERROR));
    }
}

template <>
StatusOr<std::string> Hdf5Reader::readAttribute<std::string>(const std::string& objectPath, const std::string& attrName) const {
    if (!handle_.isValid()) return StatusOr<std::string>::Fail(makeHdf5Err(Hdf5Error::HANDLE_INVALID));
    try {
        H5::Attribute a = openAttrFromPath(handle_.get(), objectPath, attrName);
        H5::DataType dt = a.getDataType();
        H5std_string s;
        a.read(dt, s);
        return StatusOr<std::string>::Ok(std::string(s.c_str()));
    } catch (const H5::AttributeIException&) {
        return StatusOr<std::string>::Fail(makeHdf5Err(Hdf5Error::ATTRIBUTE_READ_ERROR));
    } catch (const H5::Exception&) {
        return StatusOr<std::string>::Fail(makeHdf5Err(Hdf5Error::ATTRIBUTE_READ_ERROR));
    }
}

int Hdf5Reader::readAttributeWithDefault(const std::string& objectPath, const std::string& attrName, int defaultVal) const {
    if (!handle_.isValid()) return defaultVal;
    if (!attributeExists(objectPath, attrName)) return defaultVal;
    auto r = readAttribute<int>(objectPath, attrName);
    return r.ok() ? r.value() : defaultVal;
}

double Hdf5Reader::readAttributeWithDefault(const std::string& objectPath, const std::string& attrName, double defaultVal) const {
    if (!handle_.isValid()) return defaultVal;
    if (!attributeExists(objectPath, attrName)) return defaultVal;
    auto r = readAttribute<double>(objectPath, attrName);
    return r.ok() ? r.value() : defaultVal;
}

std::string Hdf5Reader::readAttributeWithDefault(const std::string& objectPath, const std::string& attrName, const std::string& defaultVal) const {
    if (!handle_.isValid()) return defaultVal;
    if (!attributeExists(objectPath, attrName)) return defaultVal;
    auto r = readAttribute<std::string>(objectPath, attrName);
    return r.ok() ? r.value() : defaultVal;
}

StatusOr<bool> Hdf5Reader::checkRequiredAttributes(const std::string& objectPath, const std::vector<std::string>& attrNames) const {
    if (!handle_.isValid()) return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::HANDLE_INVALID));
    for (const auto& name : attrNames)
        if (!attributeExists(objectPath, name))
            return StatusOr<bool>::Fail(makeHdf5Err(Hdf5Error::ATTR_NOT_EXIST));
    return StatusOr<bool>::Ok(true);
}

std::map<std::string, StatusOr<std::vector<double>>> Hdf5Reader::readDatasetsBatch(const std::vector<std::string>& paths) const {
    std::map<std::string, StatusOr<std::vector<double>>> out;
    for (const auto& path : paths)
        out.emplace(path, readDataset<std::vector<double>>(path));
    return out;
}

StatusOr<std::vector<std::string>> Hdf5Reader::getAttributeNames(const std::string& objectPath) const {
    if (!handle_.isValid()) return StatusOr<std::vector<std::string>>::Fail(makeHdf5Err(Hdf5Error::HANDLE_INVALID));
    std::string p = normPath(objectPath);
    if (!p.empty() && p != "/") return StatusOr<std::vector<std::string>>::Fail(makeHdf5Err(Hdf5Error::PATH_NOT_FOUND));
    std::vector<std::string> names;
    try {
        const_cast<H5::H5File*>(handle_.get())->iterateAttrs(collectAttrNamesOp, nullptr, &names);
        return StatusOr<std::vector<std::string>>::Ok(names);
    } catch (const H5::AttributeIException&) {
        return StatusOr<std::vector<std::string>>::Fail(makeHdf5Err(Hdf5Error::ATTRIBUTE_READ_ERROR));
    } catch (const H5::Exception&) {
        return StatusOr<std::vector<std::string>>::Fail(makeHdf5Err(Hdf5Error::ATTRIBUTE_READ_ERROR));
    }
}

StatusOr<std::vector<double>> Hdf5Reader::readDatasetSlice(const std::string& path, hsize_t offset, hsize_t count) const {
    if (!handle_.isValid()) return StatusOr<std::vector<double>>::Fail(makeHdf5Err(Hdf5Error::HANDLE_INVALID));
    const H5::H5File* fp = handle_.get();
    try {
        std::string p = normPath(path);
        H5::DataSet ds = fp->openDataSet(p);
        H5::DataSpace fspace = ds.getSpace();
        int nd = fspace.getSimpleExtentNdims();
        if (nd != 1) return StatusOr<std::vector<double>>::Fail(makeHdf5Err(Hdf5Error::DATASET_READ_ERROR));
        hsize_t dims[1] = {0};
        fspace.getSimpleExtentDims(dims);
        if (offset >= dims[0] || count == 0) {
            return StatusOr<std::vector<double>>::Ok(std::vector<double>{});
        }
        hsize_t n = count;
        if (offset + n > dims[0]) n = dims[0] - offset;
        hsize_t start[1] = {offset};
        hsize_t cnt[1] = {1};
        hsize_t blk[1] = {n};
        fspace.selectHyperslab(H5S_SELECT_SET, cnt, start, nullptr, blk);
        H5::DataSpace mspace(1, blk);
        std::vector<double> out(static_cast<size_t>(n));
        if (!out.empty())
            ds.read(out.data(), H5::PredType::NATIVE_DOUBLE, mspace, fspace);
        return StatusOr<std::vector<double>>::Ok(out);
    } catch (const H5::DataSetIException&) {
        return StatusOr<std::vector<double>>::Fail(makeHdf5Err(Hdf5Error::DATASET_READ_ERROR));
    } catch (const H5::Exception&) {
        return StatusOr<std::vector<double>>::Fail(makeHdf5Err(Hdf5Error::DATASET_READ_ERROR));
    }
}

} // namespace hdf5
} // namespace io
} // namespace common
} // namespace AIstudy


#endif // AISTUDY_HDF5_ENABLED
