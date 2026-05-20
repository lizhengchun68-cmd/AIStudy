/**
 * @file error_codes.h
 * @brief Error code definitions for industrial simulation software
 */

#ifndef AISTUDY_ERROR_CODES_H
#define AISTUDY_ERROR_CODES_H

#ifdef _WIN32
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <windows.h>
#  undef ERROR
#  undef INFO
#  undef MESH
#  undef SOLVER
#  undef CONFIG
/* Undef Windows/errno/winsock macros that conflict with enum value names */
#  ifdef CONNECTION_REFUSED
#    undef CONNECTION_REFUSED
#  endif
#  ifdef CONNECTION_FAILED
#    undef CONNECTION_FAILED
#  endif
#  ifdef TIMEOUT
#    undef TIMEOUT
#  endif
#  ifdef HOST_NOT_FOUND
#    undef HOST_NOT_FOUND
#  endif
#  ifdef FILE_NOT_FOUND
#    undef FILE_NOT_FOUND
#  endif
#  ifdef PERMISSION_DENIED
#    undef PERMISSION_DENIED
#  endif
#  ifdef DISK_FULL
#    undef DISK_FULL
#  endif
#  ifdef IO_ERROR
#    undef IO_ERROR
#  endif
#  ifdef INVALID_PATH
#    undef INVALID_PATH
#  endif
#  ifdef OUT_OF_MEMORY
#    undef OUT_OF_MEMORY
#  endif
#  ifdef INVALID_PARAMETER
#    undef INVALID_PARAMETER
#  endif
#  ifdef BUFFER_OVERFLOW
#    undef BUFFER_OVERFLOW
#  endif
#endif

#include <cstdint>

namespace AIstudy {

/**
 * @brief Error code base values for different categories
 * 
 * Each category has a base value to avoid conflicts with system error codes.
 * System error codes typically range from 1-1000, so we start from 10000.
 * Names use _BASE suffix to avoid Windows macro conflicts (e.g. MESH, SOLVER in winres).
 */
namespace ErrorCodeBase {
    // System-level errors: 10000-10999
    constexpr int32_t SYSTEM = 10000;
    constexpr int32_t NETWORK = 10100;
    constexpr int32_t FILESYSTEM = 10200;
    constexpr int32_t MEMORY = 10300;
    
    // Simulation-specific errors: 11000-11999
    constexpr int32_t SIMULATION = 11000;
    constexpr int32_t CONVERGENCE = 11100;
    constexpr int32_t BOUNDARY_CONDITION = 11200;
    constexpr int32_t MESH_BASE = 11300;
    constexpr int32_t SOLVER_BASE = 11400;
    
    // Configuration errors: 12000-12999
    constexpr int32_t CONFIG_BASE = 12000;
    constexpr int32_t PARAMETER = 12100;
    constexpr int32_t VALIDATION = 12200;
    
    // Resource errors: 13000-13999
    constexpr int32_t THREAD = 13000;
    constexpr int32_t DEVICE = 13100;
    constexpr int32_t LICENSE = 13200;

    // JSON I/O: 14000-14999
    constexpr int32_t JSON = 14000;

    // HDF5 I/O: 15000-15999
    constexpr int32_t HDF5 = 15000;
} // namespace ErrorCodeBase

/**
 * @brief System-level error codes
 */
enum class SystemError : int32_t {
    SUCCESS = 0,
    UNKNOWN_ERROR = ErrorCodeBase::SYSTEM + 1,
    INVALID_ARGUMENT = ErrorCodeBase::SYSTEM + 2,
    OPERATION_FAILED = ErrorCodeBase::SYSTEM + 3,
    NOT_IMPLEMENTED = ErrorCodeBase::SYSTEM + 4,
    OUT_OF_RANGE = ErrorCodeBase::SYSTEM + 5,
};

/**
 * @brief Network error codes
 */
enum class NetworkError : int32_t {
    CONNECTION_FAILED = ErrorCodeBase::NETWORK + 1,
    TIMEOUT = ErrorCodeBase::NETWORK + 2,
    PROTOCOL_ERROR = ErrorCodeBase::NETWORK + 3,
    HOST_NOT_FOUND = ErrorCodeBase::NETWORK + 4,
    CONNECTION_REFUSED = ErrorCodeBase::NETWORK + 5,
};

/**
 * @brief File system error codes
 */
enum class FileSystemError : int32_t {
    FILE_NOT_FOUND = ErrorCodeBase::FILESYSTEM + 1,
    PERMISSION_DENIED = ErrorCodeBase::FILESYSTEM + 2,
    DISK_FULL = ErrorCodeBase::FILESYSTEM + 3,
    IO_ERROR = ErrorCodeBase::FILESYSTEM + 4,
    INVALID_PATH = ErrorCodeBase::FILESYSTEM + 5,
};

/**
 * @brief Memory error codes
 */
enum class MemoryError : int32_t {
    ALLOCATION_FAILED = ErrorCodeBase::MEMORY + 1,
    OUT_OF_MEMORY = ErrorCodeBase::MEMORY + 2,
    INVALID_POINTER = ErrorCodeBase::MEMORY + 3,
    BUFFER_OVERFLOW = ErrorCodeBase::MEMORY + 4,
};

/**
 * @brief Simulation error codes
 */
enum class SimulationError : int32_t {
    INITIALIZATION_FAILED = ErrorCodeBase::SIMULATION + 1,
    EXECUTION_FAILED = ErrorCodeBase::SIMULATION + 2,
    INVALID_STATE = ErrorCodeBase::SIMULATION + 3,
    DATA_INCONSISTENCY = ErrorCodeBase::SIMULATION + 4,
    MODEL_ERROR = ErrorCodeBase::SIMULATION + 5,
};

/**
 * @brief Convergence error codes
 */
enum class ConvergenceError : int32_t {
    NOT_CONVERGED = ErrorCodeBase::CONVERGENCE + 1,
    MAX_ITERATIONS_REACHED = ErrorCodeBase::CONVERGENCE + 2,
    DIVERGENCE_DETECTED = ErrorCodeBase::CONVERGENCE + 3,
    SLOW_CONVERGENCE = ErrorCodeBase::CONVERGENCE + 4,
    RESIDUAL_TOO_LARGE = ErrorCodeBase::CONVERGENCE + 5,
};

/**
 * @brief Boundary condition error codes
 */
enum class BoundaryConditionError : int32_t {
    INVALID_BOUNDARY = ErrorCodeBase::BOUNDARY_CONDITION + 1,
    MISSING_BOUNDARY = ErrorCodeBase::BOUNDARY_CONDITION + 2,
    INCONSISTENT_BOUNDARY = ErrorCodeBase::BOUNDARY_CONDITION + 3,
    BOUNDARY_TYPE_MISMATCH = ErrorCodeBase::BOUNDARY_CONDITION + 4,
};

/**
 * @brief Mesh error codes
 */
enum class MeshError : int32_t {
    INVALID_MESH = ErrorCodeBase::MESH_BASE + 1,
    MESH_GENERATION_FAILED = ErrorCodeBase::MESH_BASE + 2,
    MESH_QUALITY_POOR = ErrorCodeBase::MESH_BASE + 3,
    ELEMENT_DISTORTION = ErrorCodeBase::MESH_BASE + 4,
    MESH_REFINEMENT_FAILED = ErrorCodeBase::MESH_BASE + 5,
};

/**
 * @brief Solver error codes
 */
enum class SolverError : int32_t {
    SOLVER_INITIALIZATION_FAILED = ErrorCodeBase::SOLVER_BASE + 1,
    MATRIX_SINGULAR = ErrorCodeBase::SOLVER_BASE + 2,
    PRECONDITIONER_FAILED = ErrorCodeBase::SOLVER_BASE + 3,
    ITERATIVE_SOLVER_FAILED = ErrorCodeBase::SOLVER_BASE + 4,
    LINEAR_SYSTEM_ILL_CONDITIONED = ErrorCodeBase::SOLVER_BASE + 5,
};

/**
 * @brief Configuration error codes
 */
enum class ConfigError : int32_t {
    FILE_NOT_FOUND = ErrorCodeBase::CONFIG_BASE + 1,
    INVALID_FORMAT = ErrorCodeBase::CONFIG_BASE + 2,
    MISSING_SECTION = ErrorCodeBase::CONFIG_BASE + 3,
    MISSING_KEY = ErrorCodeBase::CONFIG_BASE + 4,
    INVALID_VALUE = ErrorCodeBase::CONFIG_BASE + 5,
};

/**
 * @brief Parameter error codes
 */
enum class ParameterError : int32_t {
    INVALID_PARAMETER = ErrorCodeBase::PARAMETER + 1,
    PARAMETER_OUT_OF_RANGE = ErrorCodeBase::PARAMETER + 2,
    MISSING_PARAMETER = ErrorCodeBase::PARAMETER + 3,
    PARAMETER_TYPE_MISMATCH = ErrorCodeBase::PARAMETER + 4,
};

/**
 * @brief Validation error codes
 */
enum class ValidationError : int32_t {
    VALIDATION_FAILED = ErrorCodeBase::VALIDATION + 1,
    INVALID_INPUT = ErrorCodeBase::VALIDATION + 2,
    CONSTRAINT_VIOLATION = ErrorCodeBase::VALIDATION + 3,
    DATA_TYPE_MISMATCH = ErrorCodeBase::VALIDATION + 4,
};

/**
 * @brief Thread error codes
 */
enum class ThreadError : int32_t {
    THREAD_CREATION_FAILED = ErrorCodeBase::THREAD + 1,
    THREAD_JOIN_FAILED = ErrorCodeBase::THREAD + 2,
    DEADLOCK_DETECTED = ErrorCodeBase::THREAD + 3,
    MUTEX_ERROR = ErrorCodeBase::THREAD + 4,
};

/**
 * @brief Device error codes
 */
enum class DeviceError : int32_t {
    DEVICE_NOT_FOUND = ErrorCodeBase::DEVICE + 1,
    DEVICE_NOT_AVAILABLE = ErrorCodeBase::DEVICE + 2,
    DEVICE_INITIALIZATION_FAILED = ErrorCodeBase::DEVICE + 3,
    DEVICE_IO_ERROR = ErrorCodeBase::DEVICE + 4,
};

/**
 * @brief License error codes
 */
enum class LicenseError : int32_t {
    LICENSE_NOT_FOUND = ErrorCodeBase::LICENSE + 1,
    LICENSE_EXPIRED = ErrorCodeBase::LICENSE + 2,
    LICENSE_INVALID = ErrorCodeBase::LICENSE + 3,
    LICENSE_FEATURE_NOT_AVAILABLE = ErrorCodeBase::LICENSE + 4,
};

/**
 * @brief JSON I/O error codes
 */
enum class JsonError : int32_t {
    PARSE_ERROR = ErrorCodeBase::JSON + 1,
    SERIALIZE_ERROR = ErrorCodeBase::JSON + 2,
    FILE_READ_ERROR = ErrorCodeBase::JSON + 3,
    FILE_WRITE_ERROR = ErrorCodeBase::JSON + 4,
    INVALID_FORMAT = ErrorCodeBase::JSON + 5,
    KEY_NOT_FOUND = ErrorCodeBase::JSON + 6,
    TYPE_MISMATCH = ErrorCodeBase::JSON + 7,
};

/**
 * @brief HDF5 I/O error codes
 */
enum class Hdf5Error : int32_t {
    FILE_OPEN_ERROR = ErrorCodeBase::HDF5 + 1,
    FILE_CREATE_ERROR = ErrorCodeBase::HDF5 + 2,
    DATASET_READ_ERROR = ErrorCodeBase::HDF5 + 3,
    DATASET_WRITE_ERROR = ErrorCodeBase::HDF5 + 4,
    ATTRIBUTE_READ_ERROR = ErrorCodeBase::HDF5 + 5,
    ATTRIBUTE_WRITE_ERROR = ErrorCodeBase::HDF5 + 6,
    GROUP_CREATE_ERROR = ErrorCodeBase::HDF5 + 7,
    COMPRESSION_ERROR = ErrorCodeBase::HDF5 + 8,
    TYPE_CONVERSION_ERROR = ErrorCodeBase::HDF5 + 9,
    PATH_NOT_FOUND = ErrorCodeBase::HDF5 + 10,
    HANDLE_INVALID = ErrorCodeBase::HDF5 + 11,
    ATTR_NOT_EXIST = ErrorCodeBase::HDF5 + 12,
    DATA_CORRUPT = ErrorCodeBase::HDF5 + 13,
    DATA_LENGTH_MISMATCH = ErrorCodeBase::HDF5 + 14,
    FILE_LOCK_TIMEOUT = ErrorCodeBase::HDF5 + 15,
    DISK_FULL = ErrorCodeBase::HDF5 + 16,
};

} // namespace AIstudy

#endif // AISTUDY_ERROR_CODES_H
