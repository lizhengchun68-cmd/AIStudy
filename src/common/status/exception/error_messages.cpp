#include "common/common_status/exception/error_codes.h"
#include "common/common_status/exception/error_code_registry.h"
#include "common/common_status/exception/error_category.h"

namespace AIstudy {

/**
 * @brief Initialize default error code descriptions
 * 
 * This function registers default error messages for all predefined error codes.
 * It should be called during library initialization.
 */
void initializeDefaultErrorMessages()
{
    auto& registry = ErrorCodeRegistry::getInstance();

    // System errors
    registry.registerErrorCode(
        static_cast<int>(SystemError::UNKNOWN_ERROR),
        ErrorCategory::SYSTEM,
        "Unknown system error occurred"
    );
    registry.registerErrorCode(
        static_cast<int>(SystemError::INVALID_ARGUMENT),
        ErrorCategory::SYSTEM,
        "Invalid argument provided"
    );
    registry.registerErrorCode(
        static_cast<int>(SystemError::OPERATION_FAILED),
        ErrorCategory::SYSTEM,
        "Operation failed"
    );
    registry.registerErrorCode(
        static_cast<int>(SystemError::NOT_IMPLEMENTED),
        ErrorCategory::SYSTEM,
        "Feature not implemented"
    );
    registry.registerErrorCode(
        static_cast<int>(SystemError::OUT_OF_RANGE),
        ErrorCategory::SYSTEM,
        "Value out of valid range"
    );

    // Network errors
    registry.registerErrorCode(
        static_cast<int>(NetworkError::CONNECTION_FAILED),
        ErrorCategory::NETWORK,
        "Network connection failed"
    );
    registry.registerErrorCode(
        static_cast<int>(NetworkError::TIMEOUT),
        ErrorCategory::NETWORK,
        "Network operation timed out"
    );
    registry.registerErrorCode(
        static_cast<int>(NetworkError::PROTOCOL_ERROR),
        ErrorCategory::NETWORK,
        "Network protocol error"
    );
    registry.registerErrorCode(
        static_cast<int>(NetworkError::HOST_NOT_FOUND),
        ErrorCategory::NETWORK,
        "Host not found"
    );
    registry.registerErrorCode(
        static_cast<int>(NetworkError::CONNECTION_REFUSED),
        ErrorCategory::NETWORK,
        "Connection refused"
    );

    // File system errors
    registry.registerErrorCode(
        static_cast<int>(FileSystemError::FILE_NOT_FOUND),
        ErrorCategory::FILESYSTEM,
        "File not found"
    );
    registry.registerErrorCode(
        static_cast<int>(FileSystemError::PERMISSION_DENIED),
        ErrorCategory::FILESYSTEM,
        "Permission denied"
    );
    registry.registerErrorCode(
        static_cast<int>(FileSystemError::DISK_FULL),
        ErrorCategory::FILESYSTEM,
        "Disk full"
    );
    registry.registerErrorCode(
        static_cast<int>(FileSystemError::IO_ERROR),
        ErrorCategory::FILESYSTEM,
        "I/O error occurred"
    );
    registry.registerErrorCode(
        static_cast<int>(FileSystemError::INVALID_PATH),
        ErrorCategory::FILESYSTEM,
        "Invalid file path"
    );

    // Memory errors
    registry.registerErrorCode(
        static_cast<int>(MemoryError::ALLOCATION_FAILED),
        ErrorCategory::MEMORY,
        "Memory allocation failed"
    );
    registry.registerErrorCode(
        static_cast<int>(MemoryError::OUT_OF_MEMORY),
        ErrorCategory::MEMORY,
        "Out of memory"
    );
    registry.registerErrorCode(
        static_cast<int>(MemoryError::INVALID_POINTER),
        ErrorCategory::MEMORY,
        "Invalid pointer"
    );
    registry.registerErrorCode(
        static_cast<int>(MemoryError::BUFFER_OVERFLOW),
        ErrorCategory::MEMORY,
        "Buffer overflow detected"
    );

    // Simulation errors
    registry.registerErrorCode(
        static_cast<int>(SimulationError::INITIALIZATION_FAILED),
        ErrorCategory::SIMULATION,
        "Simulation initialization failed"
    );
    registry.registerErrorCode(
        static_cast<int>(SimulationError::EXECUTION_FAILED),
        ErrorCategory::SIMULATION,
        "Simulation execution failed"
    );
    registry.registerErrorCode(
        static_cast<int>(SimulationError::INVALID_STATE),
        ErrorCategory::SIMULATION,
        "Simulation in invalid state"
    );
    registry.registerErrorCode(
        static_cast<int>(SimulationError::DATA_INCONSISTENCY),
        ErrorCategory::SIMULATION,
        "Data inconsistency detected"
    );
    registry.registerErrorCode(
        static_cast<int>(SimulationError::MODEL_ERROR),
        ErrorCategory::SIMULATION,
        "Simulation model error"
    );

    // Convergence errors
    registry.registerErrorCode(
        static_cast<int>(ConvergenceError::NOT_CONVERGED),
        ErrorCategory::CONVERGENCE,
        "Solution did not converge"
    );
    registry.registerErrorCode(
        static_cast<int>(ConvergenceError::MAX_ITERATIONS_REACHED),
        ErrorCategory::CONVERGENCE,
        "Maximum iterations reached without convergence"
    );
    registry.registerErrorCode(
        static_cast<int>(ConvergenceError::DIVERGENCE_DETECTED),
        ErrorCategory::CONVERGENCE,
        "Solution divergence detected"
    );
    registry.registerErrorCode(
        static_cast<int>(ConvergenceError::SLOW_CONVERGENCE),
        ErrorCategory::CONVERGENCE,
        "Convergence is too slow"
    );
    registry.registerErrorCode(
        static_cast<int>(ConvergenceError::RESIDUAL_TOO_LARGE),
        ErrorCategory::CONVERGENCE,
        "Residual is too large"
    );

    // Boundary condition errors
    registry.registerErrorCode(
        static_cast<int>(BoundaryConditionError::INVALID_BOUNDARY),
        ErrorCategory::BOUNDARY_CONDITION,
        "Invalid boundary condition"
    );
    registry.registerErrorCode(
        static_cast<int>(BoundaryConditionError::MISSING_BOUNDARY),
        ErrorCategory::BOUNDARY_CONDITION,
        "Missing boundary condition"
    );
    registry.registerErrorCode(
        static_cast<int>(BoundaryConditionError::INCONSISTENT_BOUNDARY),
        ErrorCategory::BOUNDARY_CONDITION,
        "Inconsistent boundary condition"
    );
    registry.registerErrorCode(
        static_cast<int>(BoundaryConditionError::BOUNDARY_TYPE_MISMATCH),
        ErrorCategory::BOUNDARY_CONDITION,
        "Boundary condition type mismatch"
    );

    // Mesh errors
    registry.registerErrorCode(
        static_cast<int>(MeshError::INVALID_MESH),
        ErrorCategory::MESH,
        "Invalid mesh"
    );
    registry.registerErrorCode(
        static_cast<int>(MeshError::MESH_GENERATION_FAILED),
        ErrorCategory::MESH,
        "Mesh generation failed"
    );
    registry.registerErrorCode(
        static_cast<int>(MeshError::MESH_QUALITY_POOR),
        ErrorCategory::MESH,
        "Mesh quality is poor"
    );
    registry.registerErrorCode(
        static_cast<int>(MeshError::ELEMENT_DISTORTION),
        ErrorCategory::MESH,
        "Mesh element distortion detected"
    );
    registry.registerErrorCode(
        static_cast<int>(MeshError::MESH_REFINEMENT_FAILED),
        ErrorCategory::MESH,
        "Mesh refinement failed"
    );

    // Solver errors
    registry.registerErrorCode(
        static_cast<int>(SolverError::SOLVER_INITIALIZATION_FAILED),
        ErrorCategory::SOLVER,
        "Solver initialization failed"
    );
    registry.registerErrorCode(
        static_cast<int>(SolverError::MATRIX_SINGULAR),
        ErrorCategory::SOLVER,
        "Matrix is singular"
    );
    registry.registerErrorCode(
        static_cast<int>(SolverError::PRECONDITIONER_FAILED),
        ErrorCategory::SOLVER,
        "Preconditioner failed"
    );
    registry.registerErrorCode(
        static_cast<int>(SolverError::ITERATIVE_SOLVER_FAILED),
        ErrorCategory::SOLVER,
        "Iterative solver failed"
    );
    registry.registerErrorCode(
        static_cast<int>(SolverError::LINEAR_SYSTEM_ILL_CONDITIONED),
        ErrorCategory::SOLVER,
        "Linear system is ill-conditioned"
    );

    // Configuration errors
    registry.registerErrorCode(
        static_cast<int>(ConfigError::FILE_NOT_FOUND),
        ErrorCategory::CONFIG,
        "Configuration file not found"
    );
    registry.registerErrorCode(
        static_cast<int>(ConfigError::INVALID_FORMAT),
        ErrorCategory::CONFIG,
        "Invalid configuration file format"
    );
    registry.registerErrorCode(
        static_cast<int>(ConfigError::MISSING_SECTION),
        ErrorCategory::CONFIG,
        "Missing configuration section"
    );
    registry.registerErrorCode(
        static_cast<int>(ConfigError::MISSING_KEY),
        ErrorCategory::CONFIG,
        "Missing configuration key"
    );
    registry.registerErrorCode(
        static_cast<int>(ConfigError::INVALID_VALUE),
        ErrorCategory::CONFIG,
        "Invalid configuration value"
    );

    // Parameter errors
    registry.registerErrorCode(
        static_cast<int>(ParameterError::INVALID_PARAMETER),
        ErrorCategory::PARAMETER,
        "Invalid parameter"
    );
    registry.registerErrorCode(
        static_cast<int>(ParameterError::PARAMETER_OUT_OF_RANGE),
        ErrorCategory::PARAMETER,
        "Parameter value out of range"
    );
    registry.registerErrorCode(
        static_cast<int>(ParameterError::MISSING_PARAMETER),
        ErrorCategory::PARAMETER,
        "Missing required parameter"
    );
    registry.registerErrorCode(
        static_cast<int>(ParameterError::PARAMETER_TYPE_MISMATCH),
        ErrorCategory::PARAMETER,
        "Parameter type mismatch"
    );

    // Validation errors
    registry.registerErrorCode(
        static_cast<int>(ValidationError::VALIDATION_FAILED),
        ErrorCategory::VALIDATION,
        "Validation failed"
    );
    registry.registerErrorCode(
        static_cast<int>(ValidationError::INVALID_INPUT),
        ErrorCategory::VALIDATION,
        "Invalid input data"
    );
    registry.registerErrorCode(
        static_cast<int>(ValidationError::CONSTRAINT_VIOLATION),
        ErrorCategory::VALIDATION,
        "Constraint violation"
    );
    registry.registerErrorCode(
        static_cast<int>(ValidationError::DATA_TYPE_MISMATCH),
        ErrorCategory::VALIDATION,
        "Data type mismatch"
    );

    // Thread errors
    registry.registerErrorCode(
        static_cast<int>(ThreadError::THREAD_CREATION_FAILED),
        ErrorCategory::THREAD,
        "Thread creation failed"
    );
    registry.registerErrorCode(
        static_cast<int>(ThreadError::THREAD_JOIN_FAILED),
        ErrorCategory::THREAD,
        "Thread join failed"
    );
    registry.registerErrorCode(
        static_cast<int>(ThreadError::DEADLOCK_DETECTED),
        ErrorCategory::THREAD,
        "Deadlock detected"
    );
    registry.registerErrorCode(
        static_cast<int>(ThreadError::MUTEX_ERROR),
        ErrorCategory::THREAD,
        "Mutex operation error"
    );

    // Device errors
    registry.registerErrorCode(
        static_cast<int>(DeviceError::DEVICE_NOT_FOUND),
        ErrorCategory::DEVICE,
        "Device not found"
    );
    registry.registerErrorCode(
        static_cast<int>(DeviceError::DEVICE_NOT_AVAILABLE),
        ErrorCategory::DEVICE,
        "Device not available"
    );
    registry.registerErrorCode(
        static_cast<int>(DeviceError::DEVICE_INITIALIZATION_FAILED),
        ErrorCategory::DEVICE,
        "Device initialization failed"
    );
    registry.registerErrorCode(
        static_cast<int>(DeviceError::DEVICE_IO_ERROR),
        ErrorCategory::DEVICE,
        "Device I/O error"
    );

    // License errors
    registry.registerErrorCode(
        static_cast<int>(LicenseError::LICENSE_NOT_FOUND),
        ErrorCategory::LICENSE,
        "License not found"
    );
    registry.registerErrorCode(
        static_cast<int>(LicenseError::LICENSE_EXPIRED),
        ErrorCategory::LICENSE,
        "License expired"
    );
    registry.registerErrorCode(
        static_cast<int>(LicenseError::LICENSE_INVALID),
        ErrorCategory::LICENSE,
        "License is invalid"
    );
    registry.registerErrorCode(
        static_cast<int>(LicenseError::LICENSE_FEATURE_NOT_AVAILABLE),
        ErrorCategory::LICENSE,
        "License feature not available"
    );

    // HDF5 errors (checksum / consistency, HDF5_IO §4.4)
    registry.registerErrorCode(
        static_cast<int>(Hdf5Error::DATA_CORRUPT),
        ErrorCategory::HDF5,
        "HDF5 data checksum mismatch (corrupt)"
    );
    registry.registerErrorCode(
        static_cast<int>(Hdf5Error::DATA_LENGTH_MISMATCH),
        ErrorCategory::HDF5,
        "HDF5 data length mismatch"
    );
    registry.registerErrorCode(
        static_cast<int>(Hdf5Error::FILE_LOCK_TIMEOUT),
        ErrorCategory::HDF5,
        "HDF5 file lock timeout"
    );
    registry.registerErrorCode(
        static_cast<int>(Hdf5Error::DISK_FULL),
        ErrorCategory::HDF5,
        "HDF5 disk full"
    );
}

} // namespace AIstudy
