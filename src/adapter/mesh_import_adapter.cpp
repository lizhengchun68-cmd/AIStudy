#include "mesh_import_adapter.h"
#include "common/status/exception/error_category.h"
#include "common/status/exception/error_codes.h"
#include "kernel/mesh/mesh_import.h"
#include "scheduler/context_handle_rules.h"
#include "scheduler/skill_artifact_paths.h"
#include "scheduler/context_store_errors.h"
#include "scheduler/skill_execution_context.h"
#include <Poco/JSON/Object.h>
#include <Poco/JSON/Parser.h>

namespace AIstudy {
namespace adapter {
namespace mesh_import {

StatusOr<Poco::JSON::Object::Ptr> mesh_import_execute(const std::string& payload_json_str) {
    if (!scheduler::skill_execution_context::hasActiveContext()) {
        scheduler::recordContextStoreFailure(
            scheduler::formatValidationDetail("context",
                                              "no active context (envelope context required)"));
        return StatusOr<Poco::JSON::Object::Ptr>::Fail(
            ErrorCodeWrapper(static_cast<int>(ValidationError::INVALID_INPUT),
                             ErrorCategory::VALIDATION));
    }

    try {
        Poco::JSON::Parser parser;
        Poco::JSON::Object::Ptr payload =
            parser.parse(payload_json_str).extract<Poco::JSON::Object::Ptr>();
        if (!payload || !payload->has("source_path") || !payload->get("source_path").isString()) {
            return StatusOr<Poco::JSON::Object::Ptr>::Fail(
                ErrorCodeWrapper(static_cast<int>(ValidationError::INVALID_INPUT),
                                 ErrorCategory::VALIDATION));
        }

        std::string mesh_handle = "mesh_main";
        if (payload->has("mesh_handle") && payload->get("mesh_handle").isString()) {
            mesh_handle = payload->getValue<std::string>("mesh_handle");
        }
        if (!scheduler::isValidHandleId(mesh_handle)
            || scheduler::handleKindFromPrefix(mesh_handle) != scheduler::HandleKind::Mesh) {
            return StatusOr<Poco::JSON::Object::Ptr>::Fail(
                ErrorCodeWrapper(static_cast<int>(ValidationError::INVALID_INPUT),
                                 ErrorCategory::VALIDATION));
        }

        const std::string& context_id = scheduler::skill_execution_context::activeContextId();
        const std::string rel_path = mesh_handle + ".h5";
        const std::string uri = scheduler::makeArtifactUri(context_id, rel_path);
        const std::string fs_path = scheduler::artifactUriToFilesystemPath(uri);
        if (fs_path.empty() || !scheduler::ensureArtifactContextDir(context_id)) {
            return StatusOr<Poco::JSON::Object::Ptr>::Fail(
                ErrorCodeWrapper(static_cast<int>(SystemError::UNKNOWN_ERROR),
                                 ErrorCategory::SYSTEM));
        }

        const std::string source_path = payload->getValue<std::string>("source_path");
        if (!kernel::mesh::registerMeshArtifactStub(fs_path, source_path)) {
            return StatusOr<Poco::JSON::Object::Ptr>::Fail(
                ErrorCodeWrapper(static_cast<int>(SystemError::UNKNOWN_ERROR),
                                 ErrorCategory::SYSTEM));
        }

        scheduler::ArtifactMeta meta;
        meta.handle_id = mesh_handle;
        meta.kind = scheduler::HandleKind::Mesh;
        meta.uri = uri;
        const auto put = scheduler::skill_execution_context::putHandle(meta, "mesh_import");
        if (!put.ok()) {
            return StatusOr<Poco::JSON::Object::Ptr>::Fail(put.status());
        }

        Poco::JSON::Object::Ptr result(new Poco::JSON::Object);
        result->set("mesh_handle", mesh_handle);
        result->set("uri", uri);
        result->set("source_path", source_path);
        return StatusOr<Poco::JSON::Object::Ptr>::Ok(result);
    } catch (...) {
        return StatusOr<Poco::JSON::Object::Ptr>::Fail(
            ErrorCodeWrapper(static_cast<int>(JsonError::PARSE_ERROR), ErrorCategory::JSON));
    }
}

} // namespace mesh_import
} // namespace adapter
} // namespace AIstudy
