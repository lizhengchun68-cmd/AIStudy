/**
 * @file hdf5_reader.h
 * @brief HDF5 reader: openFile/closeFile, readDataset<T>, readAttribute<T>, exists checks
 *
 * Requires AISTUDY_HDF5_ENABLED and HDF5 C++ library.
 */

#ifndef AISTUDY_COMMON_IO_HDF5_HDF5_READER_H
#define AISTUDY_COMMON_IO_HDF5_HDF5_READER_H

#ifdef AISTUDY_HDF5_ENABLED

#include <map>
#include <string>
#include <vector>
#include "common/status/exception/error_code_wrapper.h"
#include "common/status/exception/error_codes.h"
#include "common/status/status_or.h"
#include "common/io/hdf5/hdf5_exception.h"
#include "common/io/hdf5/hdf5_handles.h"
#include <H5Cpp.h>

namespace AIstudy {
namespace common {
namespace io {
namespace hdf5 {


/**
 * @brief HDF5 reader (straightforward impl, no factory/PIMPL).
 * Fallible operations return StatusOr<T> only (no bool/T + err overloads).
 */
class Hdf5Reader {
public:
    Hdf5Reader() = default;
    ~Hdf5Reader();

    Hdf5Reader(const Hdf5Reader&) = delete;
    Hdf5Reader& operator=(const Hdf5Reader&) = delete;

    StatusOr<bool> openFile(const std::string& filepath);
    void closeFile();
    bool isFileOpen() const;
    const std::string& getFilePath() const;

    template <typename T>
    StatusOr<T> readDataset(const std::string& path) const;

    template <typename T>
    StatusOr<T> readAttribute(const std::string& objectPath, const std::string& attrName) const;

    bool datasetExists(const std::string& path) const;
    bool attributeExists(const std::string& objectPath, const std::string& attrName) const;
    bool groupExists(const std::string& path) const;

    StatusOr<std::vector<hsize_t>> getDatasetShape(const std::string& path) const;

    /** @brief Read attribute with default; returns default when attribute missing (HDF5_IO §4.1). */
    int readAttributeWithDefault(const std::string& objectPath, const std::string& attrName, int defaultVal) const;
    double readAttributeWithDefault(const std::string& objectPath, const std::string& attrName, double defaultVal) const;
    std::string readAttributeWithDefault(const std::string& objectPath, const std::string& attrName, const std::string& defaultVal) const;

    /** @brief Check required attributes exist; missing any �?Fail(ATTR_NOT_EXIST) (HDF5_IO §4.1). */
    StatusOr<bool> checkRequiredAttributes(const std::string& objectPath, const std::vector<std::string>& attrNames) const;

    /** @brief Batch read 1D double datasets (HDF5_IO §5.3). Returns path �?StatusOr. */
    std::map<std::string, StatusOr<std::vector<double>>> readDatasetsBatch(const std::vector<std::string>& paths) const;

    /** @brief List attribute names at \a objectPath (root \"/\" only for now). */
    StatusOr<std::vector<std::string>> getAttributeNames(const std::string& objectPath) const;

    /** @brief Read 1D double slice [offset, offset+count) (HDF5_IO §6.1, §6.3 chunked). */
    StatusOr<std::vector<double>> readDatasetSlice(const std::string& path, hsize_t offset, hsize_t count) const;

private:
    std::string filepath_;
    H5FileHandle handle_;
};

template <> StatusOr<int> Hdf5Reader::readDataset<int>(const std::string& path) const;
template <> StatusOr<double> Hdf5Reader::readDataset<double>(const std::string& path) const;
template <> StatusOr<float> Hdf5Reader::readDataset<float>(const std::string& path) const;
template <> StatusOr<std::string> Hdf5Reader::readDataset<std::string>(const std::string& path) const;

/** @brief 1D array (HDF5_IO §3.2). Reads 1D dataset into std::vector. */
template <> StatusOr<std::vector<int>> Hdf5Reader::readDataset<std::vector<int>>(const std::string& path) const;
template <> StatusOr<std::vector<double>> Hdf5Reader::readDataset<std::vector<double>>(const std::string& path) const;
template <> StatusOr<std::vector<float>> Hdf5Reader::readDataset<std::vector<float>>(const std::string& path) const;

/** @brief 2D array (HDF5_IO §3.2). Reads 2D dataset into std::vector<std::vector<T>> (row-major). */
template <> StatusOr<std::vector<std::vector<int>>> Hdf5Reader::readDataset<std::vector<std::vector<int>>>(const std::string& path) const;
template <> StatusOr<std::vector<std::vector<double>>> Hdf5Reader::readDataset<std::vector<std::vector<double>>>(const std::string& path) const;
template <> StatusOr<std::vector<std::vector<float>>> Hdf5Reader::readDataset<std::vector<std::vector<float>>>(const std::string& path) const;

/** @brief 3D array (HDF5_IO §3.2). Reads 3D dataset into std::vector<std::vector<std::vector<T>>> (row-major). */
template <> StatusOr<std::vector<std::vector<std::vector<int>>>> Hdf5Reader::readDataset<std::vector<std::vector<std::vector<int>>>>(const std::string& path) const;
template <> StatusOr<std::vector<std::vector<std::vector<double>>>> Hdf5Reader::readDataset<std::vector<std::vector<std::vector<double>>>>(const std::string& path) const;
template <> StatusOr<std::vector<std::vector<std::vector<float>>>> Hdf5Reader::readDataset<std::vector<std::vector<std::vector<float>>>>(const std::string& path) const;

/** @brief 1D string array (HDF5_IO §3.3). Variable-length UTF-8. */
template <> StatusOr<std::vector<std::string>> Hdf5Reader::readDataset<std::vector<std::string>>(const std::string& path) const;

template <> StatusOr<int> Hdf5Reader::readAttribute<int>(const std::string& objectPath, const std::string& attrName) const;
template <> StatusOr<double> Hdf5Reader::readAttribute<double>(const std::string& objectPath, const std::string& attrName) const;
template <> StatusOr<float> Hdf5Reader::readAttribute<float>(const std::string& objectPath, const std::string& attrName) const;
template <> StatusOr<std::string> Hdf5Reader::readAttribute<std::string>(const std::string& objectPath, const std::string& attrName) const;

} // namespace hdf5
} // namespace io
} // namespace common
} // namespace AIstudy


#endif // AISTUDY_HDF5_ENABLED

#endif // AISTUDY_COMMON_IO_HDF5_HDF5_READER_H
