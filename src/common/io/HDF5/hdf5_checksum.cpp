#ifdef AISTUDY_HDF5_ENABLED

#include "common/io/hdf5/hdf5_checksum.h"
#include "common/status/exception/error_codes.h"
#include "common/status/exception/error_category.h"
#include <cstring>
#include <sstream>
#include <iomanip>

namespace AIstudy {
namespace common {
namespace io {
namespace hdf5 {


namespace {

constexpr uint64_t kFnvOffset = 0xcbf29ce484222325ULL;
constexpr uint64_t kFnvPrime = 0x100000001b3ULL;

uint64_t fnv1a64(const unsigned char* p, size_t n) {
    uint64_t h = kFnvOffset;
    for (size_t i = 0; i < n; ++i) {
        h ^= static_cast<uint64_t>(p[i]);
        h *= kFnvPrime;
    }
    return h;
}

std::string toHex(uint64_t v) {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0') << std::setw(16) << v;
    return oss.str();
}

bool fromHex(const std::string& s, uint64_t& out) {
    if (s.size() > 16u) return false;
    uint64_t v = 0;
    for (char c : s) {
        int d = 0;
        if (c >= '0' && c <= '9') d = c - '0';
        else if (c >= 'a' && c <= 'f') d = c - 'a' + 10;
        else if (c >= 'A' && c <= 'F') d = c - 'A' + 10;
        else return false;
        v = (v << 4) | static_cast<uint64_t>(d);
    }
    out = v;
    return true;
}

} // namespace

uint64_t computeChecksum64(const std::vector<double>& vec) {
    if (vec.empty()) return fnv1a64(nullptr, 0);
    return fnv1a64(reinterpret_cast<const unsigned char*>(vec.data()),
                   vec.size() * sizeof(double));
}

StatusOr<bool> writeDatasetWithChecksum(Hdf5Writer& w, const std::string& path,
                                        const std::vector<double>& data) {
    auto wr = w.writeDataset(path, data);
    if (!wr.ok()) return wr;
    uint64_t cs = computeChecksum64(data);
    std::string hex = toHex(cs);
    auto aw = w.writeAttribute(path, "checksum", hex);
    if (!aw.ok()) return aw;
    return StatusOr<bool>::Ok(true);
}

StatusOr<std::vector<double>> readDatasetWithVerify(
    const Hdf5Reader& r, const std::string& path, int expectedSize) {
    auto res = r.readDataset<std::vector<double>>(path);
    if (!res.ok()) return res;
    std::vector<double> data = std::move(res.value());

    if (expectedSize >= 0 && static_cast<int>(data.size()) != expectedSize)
        return StatusOr<std::vector<double>>::Fail(
            ErrorCodeWrapper(static_cast<int>(Hdf5Error::DATA_LENGTH_MISMATCH),
                            ErrorCategory::HDF5));

    if (!r.attributeExists(path, "checksum"))
        return StatusOr<std::vector<double>>::Fail(
            ErrorCodeWrapper(static_cast<int>(Hdf5Error::DATA_CORRUPT),
                            ErrorCategory::HDF5));

    auto attr = r.readAttribute<std::string>(path, "checksum");
    if (!attr.ok()) return StatusOr<std::vector<double>>::Fail(attr.status());
    uint64_t stored = 0;
    if (!fromHex(attr.value(), stored))
        return StatusOr<std::vector<double>>::Fail(
            ErrorCodeWrapper(static_cast<int>(Hdf5Error::DATA_CORRUPT),
                            ErrorCategory::HDF5));

    uint64_t computed = computeChecksum64(data);
    if (computed != stored)
        return StatusOr<std::vector<double>>::Fail(
            ErrorCodeWrapper(static_cast<int>(Hdf5Error::DATA_CORRUPT),
                            ErrorCategory::HDF5));

    return StatusOr<std::vector<double>>::Ok(std::move(data));
}

} // namespace hdf5
} // namespace io
} // namespace common
} // namespace AIstudy

#endif // AISTUDY_HDF5_ENABLED
