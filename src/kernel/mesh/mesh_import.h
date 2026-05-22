#ifndef AISTUDY_KERNEL_MESH_MESH_IMPORT_H
#define AISTUDY_KERNEL_MESH_MESH_IMPORT_H

#include <string>

namespace AIstudy {
namespace kernel {
namespace mesh {

/** @brief M5b stub: register mesh artifact path (real importer later). */
bool registerMeshArtifactStub(const std::string& filesystem_path,
                              const std::string& source_path);

} // namespace mesh
} // namespace kernel
} // namespace AIstudy

#endif // AISTUDY_KERNEL_MESH_MESH_IMPORT_H
