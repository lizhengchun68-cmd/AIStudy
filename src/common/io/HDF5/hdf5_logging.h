/**
 * @file hdf5_logging.h
 * @brief HDF5 operation logging and tracing (HDF5_IO §5.5, §6.2)
 *
 * Optional logging of HDF5 ops (path, success/failure, duration) via Logger.
 * Enable via setHdf5OperationLogging / setHdf5OperationTiming.
 */

#ifndef AISTUDY_COMMON_IO_HDF5_HDF5_LOGGING_H
#define AISTUDY_COMMON_IO_HDF5_HDF5_LOGGING_H

#ifdef AISTUDY_HDF5_ENABLED

#include <chrono>
#include <string>

namespace AIstudy {
namespace common {
namespace io {
namespace hdf5 {


/** @brief Enable/disable HDF5 operation logging (INFO success, ERROR failure). Default: off. */
void setHdf5OperationLogging(bool enabled);
bool isHdf5OperationLoggingEnabled();

/** @brief Enable/disable recording operation duration (ms) in logs. Default: off. */
void setHdf5OperationTiming(bool enabled);
bool isHdf5OperationTimingEnabled();

/**
 * @brief Log one HDF5 operation. No-op if logging disabled.
 * @param op       Operation name (e.g. "createFile", "writeDataset")
 * @param path     Object path (file path, dataset path, or "obj@attr" for attributes)
 * @param ok       true = success, false = failure
 * @param duration Elapsed time (use 0 if timing disabled)
 * @param detail   Extra info (e.g. "len=42", error message, "attr=name")
 */
void logHdf5Op(const char* op, const std::string& path, bool ok,
               std::chrono::microseconds duration, const std::string& detail = "");

/**
 * @brief RAII guard: start time on ctor, log on dtor. Call fail(detail) before error returns.
 */
struct Hdf5OpTrace {
    const char* op;
    std::string path;
    bool ok = true;
    std::string detail;
    std::chrono::steady_clock::time_point t0;

    explicit Hdf5OpTrace(const char* operation, std::string p)
        : op(operation), path(std::move(p)), t0(std::chrono::steady_clock::now()) {}

    void fail(std::string d) { ok = false; detail = std::move(d); }
    void setDetail(std::string d) { detail = std::move(d); }

    ~Hdf5OpTrace();
};

} // namespace hdf5
} // namespace io
} // namespace common
} // namespace AIstudy


#endif // AISTUDY_HDF5_ENABLED

#endif // AISTUDY_COMMON_IO_HDF5_HDF5_LOGGING_H
