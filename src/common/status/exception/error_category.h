/**
 * @file error_category.h
 * @brief Error category definitions for simulation software
 */

#ifndef AISTUDY_ERROR_CATEGORY_H
#define AISTUDY_ERROR_CATEGORY_H

#ifdef _WIN32
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  include <windows.h>
#  undef MESH
#  undef SOLVER
#  undef CONFIG
#endif

#include <string>

namespace AIstudy {

/**
 * @brief Error categories for industrial simulation software
 */
namespace ErrorCategory {
    // System-level errors
    constexpr const char* SYSTEM = "system";
    constexpr const char* NETWORK = "network";
    constexpr const char* FILESYSTEM = "filesystem";
    constexpr const char* MEMORY = "memory";
    
    // Simulation-specific errors
    constexpr const char* SIMULATION = "simulation";
    constexpr const char* CONVERGENCE = "convergence";
    constexpr const char* BOUNDARY_CONDITION = "boundary_condition";
    constexpr const char* MESH = "mesh";
    constexpr const char* SOLVER = "solver";
    
    // Configuration errors
    constexpr const char* CONFIG = "config";
    constexpr const char* PARAMETER = "parameter";
    constexpr const char* VALIDATION = "validation";
    
    // Resource errors
    constexpr const char* THREAD = "thread";
    constexpr const char* DEVICE = "device";
    constexpr const char* LICENSE = "license";

    // I/O errors
    constexpr const char* JSON = "json";
    constexpr const char* HDF5 = "hdf5";
} // namespace ErrorCategory

/**
 * @brief Get error category description
 * @param category Category name
 * @return Human-readable description
 */
std::string getCategoryDescription(const std::string& category);

} // namespace AIstudy

#endif // AISTUDY_ERROR_CATEGORY_H
