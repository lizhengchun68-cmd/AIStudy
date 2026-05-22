#include "kernel/mesh/mesh_import.h"
#include <fstream>

namespace AIstudy {
namespace kernel {
namespace mesh {

bool registerMeshArtifactStub(const std::string& filesystem_path,
                              const std::string& source_path) {
    std::ofstream out(filesystem_path, std::ios::binary | std::ios::trunc);
    if (!out) {
        return false;
    }
    out << "# AIstudy mesh artifact stub\n";
    out << "source_path=" << source_path << "\n";
    return out.good();
}

} // namespace mesh
} // namespace kernel
} // namespace AIstudy
