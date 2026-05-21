#ifdef AISTUDY_HDF5_ENABLED

#include "common/io/hdf5/hdf5_factory.h"
#include "common/io/hdf5/hdf5_reader.h"
#include "common/io/hdf5/hdf5_writer.h"
#include "common/common_status/exception/error_category.h"
#include "common/common_status/exception/error_codes.h"
#include "common/common_status/status_or.h"
#include <H5Cpp.h>
#include <memory>

namespace AIstudy {
namespace common {
namespace io {
namespace hdf5 {


namespace {

ErrorCodeWrapper makeHdf5Err(Hdf5Error e) {
    return ErrorCodeWrapper(static_cast<int>(e), ErrorCategory::HDF5);
}

} // namespace

StatusOr<H5FileHandle> H5Factory::OpenFile(const std::string& filepath) {
    try {
        H5::Exception::dontPrint();
        auto p = std::unique_ptr<H5::H5File>(new H5::H5File(filepath, H5F_ACC_RDONLY));
        return StatusOr<H5FileHandle>::Ok(H5FileHandle(std::move(p)));
    } catch (const H5::FileIException&) {
        return StatusOr<H5FileHandle>::Fail(makeHdf5Err(Hdf5Error::FILE_OPEN_ERROR));
    } catch (const H5::Exception&) {
        return StatusOr<H5FileHandle>::Fail(makeHdf5Err(Hdf5Error::FILE_OPEN_ERROR));
    }
}

StatusOr<H5FileHandle> H5Factory::CreateFile(const std::string& filepath) {
    try {
        H5::Exception::dontPrint();
        auto p = std::unique_ptr<H5::H5File>(new H5::H5File(filepath, H5F_ACC_TRUNC));
        return StatusOr<H5FileHandle>::Ok(H5FileHandle(std::move(p)));
    } catch (const H5::FileIException&) {
        return StatusOr<H5FileHandle>::Fail(makeHdf5Err(Hdf5Error::FILE_CREATE_ERROR));
    } catch (const H5::Exception&) {
        return StatusOr<H5FileHandle>::Fail(makeHdf5Err(Hdf5Error::FILE_CREATE_ERROR));
    }
}

StatusOr<H5FileHandle> H5Factory::OpenFileReadWrite(const std::string& filepath, bool createIfNotExists) {
    try {
        H5::Exception::dontPrint();
        auto p = std::unique_ptr<H5::H5File>(new H5::H5File(filepath, H5F_ACC_RDWR));
        return StatusOr<H5FileHandle>::Ok(H5FileHandle(std::move(p)));
    } catch (const H5::FileIException&) {
        if (createIfNotExists) {
            try {
                auto q = std::unique_ptr<H5::H5File>(new H5::H5File(filepath, H5F_ACC_TRUNC));
                return StatusOr<H5FileHandle>::Ok(H5FileHandle(std::move(q)));
            } catch (const H5::Exception&) {
                return StatusOr<H5FileHandle>::Fail(makeHdf5Err(Hdf5Error::FILE_CREATE_ERROR));
            }
        }
        return StatusOr<H5FileHandle>::Fail(makeHdf5Err(Hdf5Error::FILE_OPEN_ERROR));
    } catch (const H5::Exception&) {
        return StatusOr<H5FileHandle>::Fail(makeHdf5Err(Hdf5Error::FILE_OPEN_ERROR));
    }
}

StatusOr<bool> H5Factory::WriteDataset(const std::string& filepath, const std::string& ds_path,
                                       const std::vector<double>& data) {
    Hdf5Writer w;
    auto open = w.openFile(filepath, true);
    if (!open.ok()) return StatusOr<bool>::Fail(open.status());
    auto wr = w.writeDataset(ds_path, data);
    w.closeFile();
    return wr;
}

StatusOr<bool> H5Factory::WriteDataset(const std::string& filepath, const std::string& ds_path,
                                       const std::vector<int>& data) {
    Hdf5Writer w;
    auto open = w.openFile(filepath, true);
    if (!open.ok()) return StatusOr<bool>::Fail(open.status());
    auto wr = w.writeDataset(ds_path, data);
    w.closeFile();
    return wr;
}

StatusOr<std::vector<double>> H5Factory::ReadDatasetDouble(const std::string& filepath, const std::string& ds_path) {
    Hdf5Reader r;
    auto open = r.openFile(filepath);
    if (!open.ok()) return StatusOr<std::vector<double>>::Fail(open.status());
    auto res = r.readDataset<std::vector<double>>(ds_path);
    r.closeFile();
    return res;
}

StatusOr<std::vector<int>> H5Factory::ReadDatasetInt(const std::string& filepath, const std::string& ds_path) {
    Hdf5Reader r;
    auto open = r.openFile(filepath);
    if (!open.ok()) return StatusOr<std::vector<int>>::Fail(open.status());
    auto res = r.readDataset<std::vector<int>>(ds_path);
    r.closeFile();
    return res;
}

StatusOr<bool> H5Factory::WriteAttribute(const std::string& filepath, const std::string& obj_path,
                                         const std::string& attr_name, double value) {
    Hdf5Writer w;
    auto open = w.openFile(filepath, true);
    if (!open.ok()) return StatusOr<bool>::Fail(open.status());
    auto wr = w.writeAttribute(obj_path, attr_name, value);
    w.closeFile();
    return wr;
}

StatusOr<bool> H5Factory::WriteAttribute(const std::string& filepath, const std::string& obj_path,
                                         const std::string& attr_name, int value) {
    Hdf5Writer w;
    auto open = w.openFile(filepath, true);
    if (!open.ok()) return StatusOr<bool>::Fail(open.status());
    auto wr = w.writeAttribute(obj_path, attr_name, value);
    w.closeFile();
    return wr;
}

StatusOr<bool> H5Factory::WriteAttribute(const std::string& filepath, const std::string& obj_path,
                                         const std::string& attr_name, const std::string& value) {
    Hdf5Writer w;
    auto open = w.openFile(filepath, true);
    if (!open.ok()) return StatusOr<bool>::Fail(open.status());
    auto wr = w.writeAttribute(obj_path, attr_name, value);
    w.closeFile();
    return wr;
}

} // namespace hdf5
} // namespace io
} // namespace common
} // namespace AIstudy


#endif // AISTUDY_HDF5_ENABLED
