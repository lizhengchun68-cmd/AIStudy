#ifdef AISTUDY_HDF5_ENABLED

#include "common/io/hdf5/hdf5_logging.h"
#include <Poco/Format.h>
#include <Poco/Logger.h>
#include <mutex>

namespace AIstudy {
namespace common {
namespace io {
namespace hdf5 {


namespace {

static bool s_logging = false;
static bool s_timing = false;
static std::mutex s_mtx;

} // namespace

void setHdf5OperationLogging(bool enabled) {
    std::lock_guard<std::mutex> lk(s_mtx);
    s_logging = enabled;
}

bool isHdf5OperationLoggingEnabled() {
    std::lock_guard<std::mutex> lk(s_mtx);
    return s_logging;
}

void setHdf5OperationTiming(bool enabled) {
    std::lock_guard<std::mutex> lk(s_mtx);
    s_timing = enabled;
}

bool isHdf5OperationTimingEnabled() {
    std::lock_guard<std::mutex> lk(s_mtx);
    return s_timing;
}

void logHdf5Op(const char* op, const std::string& path, bool ok,
               std::chrono::microseconds duration, const std::string& detail) {
    bool doLog = false;
    bool doTime = false;
    {
        std::lock_guard<std::mutex> lk(s_mtx);
        doLog = s_logging;
        doTime = s_timing;
    }
    if (!doLog) return;

    Poco::Logger& log = Poco::Logger::get("AIstudy.HDF5");
    std::string msg = Poco::format("%s path=%s", std::string(op), path);
    if (!detail.empty()) msg += " " + detail;
    if (doTime)
        msg += Poco::format(" duration_us=%ld", static_cast<long>(duration.count()));

    if (ok)
        log.information(msg);
    else
        log.error(msg);
}

Hdf5OpTrace::~Hdf5OpTrace() {
    auto d = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now() - t0);
    logHdf5Op(op, path, ok, d, detail);
}

} // namespace hdf5
} // namespace io
} // namespace common
} // namespace AIstudy


#endif // AISTUDY_HDF5_ENABLED
