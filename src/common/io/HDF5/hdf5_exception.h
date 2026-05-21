/**
 * @file hdf5_exception.h
 * @brief HDF5 I/O exceptions (file, dataset, attribute)
 */

#ifndef AISTUDY_COMMON_IO_HDF5_HDF5_EXCEPTION_H
#define AISTUDY_COMMON_IO_HDF5_HDF5_EXCEPTION_H

#include "common/status/exception/exception.h"

namespace AIstudy {
namespace common {
namespace io {
namespace hdf5 {


/**
 * @brief HDF5 generic exception
 */
class Hdf5Exception : public SimUtilsException {
public:
    Hdf5Exception(const std::string& message, const ErrorCodeWrapper& errorCode,
                  const ExceptionContext& context = {})
        : SimUtilsException(message, errorCode, context) {}
};

/**
 * @brief HDF5 file open/create I/O exception
 */
class Hdf5FileException : public Hdf5Exception {
public:
    Hdf5FileException(const std::string& message, const ErrorCodeWrapper& errorCode,
                      const ExceptionContext& context = {})
        : Hdf5Exception(message, errorCode, context) {}
};

/**
 * @brief HDF5 dataset read/write exception
 */
class Hdf5DatasetException : public Hdf5Exception {
public:
    Hdf5DatasetException(const std::string& message, const ErrorCodeWrapper& errorCode,
                         const ExceptionContext& context = {})
        : Hdf5Exception(message, errorCode, context) {}
};

/**
 * @brief HDF5 attribute read/write exception
 */
class Hdf5AttributeException : public Hdf5Exception {
public:
    Hdf5AttributeException(const std::string& message, const ErrorCodeWrapper& errorCode,
                           const ExceptionContext& context = {})
        : Hdf5Exception(message, errorCode, context) {}
};

} // namespace hdf5
} // namespace io
} // namespace common
} // namespace AIstudy


#endif // AISTUDY_COMMON_IO_HDF5_HDF5_EXCEPTION_H
