/**
 * @file exception.h
 * @brief Custom exception classes for error handling
 */

#ifndef AISTUDY_EXCEPTION_H
#define AISTUDY_EXCEPTION_H

#include <exception>
#include <stdexcept>
#include <string>
#include <vector>
#include "error_code_wrapper.h"
#include "error_info.h"

namespace AIstudy {

/**
 * @brief Base exception class for AIstudy library
 */
class SimUtilsException : public std::runtime_error {
public:
    /**
     * @brief Constructor
     * @param message Error message
     * @param errorCode Associated error code
     * @param context Optional exception context (file, line, function)
     */
    SimUtilsException(const std::string& message, const ErrorCodeWrapper& errorCode,
                      const ExceptionContext& context = {});

    /**
     * @brief Get the associated error code
     * @return Error code wrapper
     */
    const ErrorCodeWrapper& errorCode() const noexcept;

    /**
     * @brief Convert to unified ErrorInfo
     */
    ErrorInfo toErrorInfo() const;

    /** @brief Set cause (exception chain). */
    void setCause(std::exception_ptr p) noexcept;
    /** @brief Get cause; null if none. */
    std::exception_ptr getCause() const noexcept;
    /** @brief Walk chain: [this->what(), cause->what(), ...]. */
    std::vector<std::string> getExceptionChain() const;

private:
    ErrorCodeWrapper  errorCode_;
    ExceptionContext  context_;
    std::exception_ptr cause_;
};

/**
 * @brief Simulation-specific exception
 */
class SimulationException : public SimUtilsException {
public:
    SimulationException(const std::string& message, const ErrorCodeWrapper& errorCode,
                        const ExceptionContext& context = {})
        : SimUtilsException(message, errorCode, context) {}
};

/**
 * @brief Configuration exception
 */
class ConfigurationException : public SimUtilsException {
public:
    ConfigurationException(const std::string& message, const ErrorCodeWrapper& errorCode,
                           const ExceptionContext& context = {})
        : SimUtilsException(message, errorCode, context) {}
};

/**
 * @brief Network exception
 */
class NetworkException : public SimUtilsException {
public:
    NetworkException(const std::string& message, const ErrorCodeWrapper& errorCode,
                     const ExceptionContext& context = {})
        : SimUtilsException(message, errorCode, context) {}
};

/**
 * @brief File system exception
 */
class FileSystemException : public SimUtilsException {
public:
    FileSystemException(const std::string& message, const ErrorCodeWrapper& errorCode,
                        const ExceptionContext& context = {})
        : SimUtilsException(message, errorCode, context) {}
};

/**
 * @brief Memory exception
 */
class MemoryException : public SimUtilsException {
public:
    MemoryException(const std::string& message, const ErrorCodeWrapper& errorCode,
                    const ExceptionContext& context = {})
        : SimUtilsException(message, errorCode, context) {}
};

/**
 * @brief Convergence exception
 */
class ConvergenceException : public SimUtilsException {
public:
    ConvergenceException(const std::string& message, const ErrorCodeWrapper& errorCode,
                         const ExceptionContext& context = {})
        : SimUtilsException(message, errorCode, context) {}
};

/**
 * @brief Solver exception
 */
class SolverException : public SimUtilsException {
public:
    SolverException(const std::string& message, const ErrorCodeWrapper& errorCode,
                    const ExceptionContext& context = {})
        : SimUtilsException(message, errorCode, context) {}
};

/**
 * @brief Mesh exception
 */
class MeshException : public SimUtilsException {
public:
    MeshException(const std::string& message, const ErrorCodeWrapper& errorCode,
                  const ExceptionContext& context = {})
        : SimUtilsException(message, errorCode, context) {}
};

/**
 * @brief Parameter exception
 */
class ParameterException : public SimUtilsException {
public:
    ParameterException(const std::string& message, const ErrorCodeWrapper& errorCode,
                       const ExceptionContext& context = {})
        : SimUtilsException(message, errorCode, context) {}
};

/**
 * @brief Validation exception
 */
class ValidationException : public SimUtilsException {
public:
    ValidationException(const std::string& message, const ErrorCodeWrapper& errorCode,
                        const ExceptionContext& context = {})
        : SimUtilsException(message, errorCode, context) {}
};

} // namespace AIstudy

#endif // AISTUDY_EXCEPTION_H
