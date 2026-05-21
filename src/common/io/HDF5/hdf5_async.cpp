#ifdef AISTUDY_HDF5_ENABLED

#include "common/io/hdf5/hdf5_async.h"
#include "common/io/hdf5/hdf5_reader.h"
#include "common/io/hdf5/hdf5_writer.h"

namespace AIstudy {
namespace common {
namespace io {
namespace hdf5 {


std::future<StatusOr<std::vector<double>>> readDatasetAsync(const std::string& filePath,
                                                            const std::string& path) {
    return std::async(std::launch::async, [filePath, path]() {
        Hdf5Reader r;
        auto open = r.openFile(filePath);
        if (!open.ok()) return StatusOr<std::vector<double>>::Fail(open.status());
        return r.readDataset<std::vector<double>>(path);
    });
}

std::future<StatusOr<bool>> writeDatasetAsync(const std::string& filePath,
                                              const std::string& path,
                                              const std::vector<double>& data) {
    std::vector<double> copy = data;
    return std::async(std::launch::async, [filePath, path, copy]() {
        Hdf5Writer w;
        auto cr = w.openFile(filePath, true);
        if (!cr.ok()) return StatusOr<bool>::Fail(cr.status());
        return w.writeDataset(path, copy);
    });
}

} // namespace hdf5
} // namespace io
} // namespace common
} // namespace AIstudy


#endif // AISTUDY_HDF5_ENABLED
