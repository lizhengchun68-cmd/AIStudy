#include "common/common_status/exception/error_category.h"
#include <map>

namespace AIstudy {

std::string getCategoryDescription(const std::string& category)
{
    static const std::map<std::string, std::string> descriptions = {
        {ErrorCategory::SYSTEM, "System-level error"},
        {ErrorCategory::NETWORK, "Network communication error"},
        {ErrorCategory::FILESYSTEM, "File system operation error"},
        {ErrorCategory::MEMORY, "Memory allocation or access error"},
        {ErrorCategory::SIMULATION, "Simulation execution error"},
        {ErrorCategory::CONVERGENCE, "Numerical convergence error"},
        {ErrorCategory::BOUNDARY_CONDITION, "Boundary condition error"},
        {ErrorCategory::MESH, "Mesh generation or processing error"},
        {ErrorCategory::SOLVER, "Solver execution error"},
        {ErrorCategory::CONFIG, "Configuration error"},
        {ErrorCategory::PARAMETER, "Parameter validation error"},
        {ErrorCategory::VALIDATION, "Input validation error"},
        {ErrorCategory::THREAD, "Threading or concurrency error"},
        {ErrorCategory::DEVICE, "Hardware device error"},
        {ErrorCategory::LICENSE, "License or authorization error"},
        {ErrorCategory::JSON, "JSON file I/O error"},
        {ErrorCategory::HDF5, "HDF5 file I/O error"},
    };
    
    auto it = descriptions.find(category);
    if (it != descriptions.end()) {
        return it->second;
    }
    
    return "Unknown error category: " + category;
}

} // namespace AIstudy
