#include "scheduler/context_handle_rules.h"
#include "common/status/exception/error_category.h"
#include "common/status/exception/error_codes.h"
#include <Poco/JSON/Object.h>
#include <cctype>

namespace AIstudy {
namespace scheduler {
namespace {

ErrorCodeWrapper validationFail() {
    return ErrorCodeWrapper(static_cast<int>(ValidationError::INVALID_INPUT),
                            ErrorCategory::VALIDATION);
}

StatusOr<ArtifactMeta> failMeta(const std::string& /*path*/, const std::string& /*reason*/) {
    return StatusOr<ArtifactMeta>::Fail(validationFail());
}

bool isAllowedIdChar(char c) {
    return std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_' || c == '.' || c == '-';
}

} // namespace

const char* handleKindToString(HandleKind kind) {
    switch (kind) {
    case HandleKind::Mesh:
        return "mesh";
    case HandleKind::Result:
        return "result";
    case HandleKind::File:
        return "file";
    default:
        return "unknown";
    }
}

HandleKind handleKindFromPrefix(const std::string& handle_id) {
    if (handle_id.rfind("mesh_", 0) == 0) {
        return HandleKind::Mesh;
    }
    if (handle_id.rfind("result_", 0) == 0) {
        return HandleKind::Result;
    }
    if (handle_id.rfind("file_", 0) == 0) {
        return HandleKind::File;
    }
    return HandleKind::Unknown;
}

bool isValidContextId(const std::string& context_id) {
    if (context_id.empty() || context_id.size() > 64) {
        return false;
    }
    for (char c : context_id) {
        if (!isAllowedIdChar(c)) {
            return false;
        }
    }
    return true;
}

bool isValidHandleId(const std::string& handle_id) {
    if (handle_id.empty() || handle_id.size() > 128) {
        return false;
    }
    const HandleKind kind = handleKindFromPrefix(handle_id);
    if (kind == HandleKind::Unknown) {
        return false;
    }
    for (size_t i = 0; i < handle_id.size(); ++i) {
        if (!isAllowedIdChar(handle_id[i])) {
            return false;
        }
    }
    return true;
}

bool handleKindMatchesId(const std::string& handle_id, HandleKind kind) {
    if (kind == HandleKind::Unknown) {
        return true;
    }
    return handleKindFromPrefix(handle_id) == kind;
}

HandleKind parseKindString(const std::string& s) {
    if (s == "mesh") {
        return HandleKind::Mesh;
    }
    if (s == "result") {
        return HandleKind::Result;
    }
    if (s == "file") {
        return HandleKind::File;
    }
    return HandleKind::Unknown;
}

StatusOr<ArtifactMeta> parseHandleEntry(const std::string& handle_id,
                                        const Poco::JSON::Object::Ptr& obj,
                                        const std::string& path_prefix) {
    if (!isValidHandleId(handle_id)) {
        return failMeta(path_prefix, "invalid handle_id prefix or charset");
    }
    ArtifactMeta meta;
    meta.handle_id = handle_id;
    meta.kind = handleKindFromPrefix(handle_id);

    if (obj) {
        if (obj->has("kind")) {
            const std::string kind_str = obj->getValue<std::string>("kind");
            const HandleKind declared = parseKindString(kind_str);
            if (declared == HandleKind::Unknown) {
                return failMeta(path_prefix, "unknown kind");
            }
            if (!handleKindMatchesId(handle_id, declared)) {
                return failMeta(path_prefix, "kind does not match handle_id prefix");
            }
            meta.kind = declared;
        }
        if (obj->has("uri")) {
            meta.uri = obj->getValue<std::string>("uri");
        }
    }
    return StatusOr<ArtifactMeta>::Ok(std::move(meta));
}

StatusOr<std::pair<std::string, std::vector<ArtifactMeta>>> parseContextObject(
    const Poco::JSON::Object::Ptr& context_obj) {
    if (!context_obj) {
        return StatusOr<std::pair<std::string, std::vector<ArtifactMeta>>>::Ok(
            std::make_pair(std::string(), std::vector<ArtifactMeta>()));
    }

    if (!context_obj->has("context_id")) {
        return StatusOr<std::pair<std::string, std::vector<ArtifactMeta>>>::Fail(validationFail());
    }

    const std::string context_id = context_obj->getValue<std::string>("context_id");
    if (!isValidContextId(context_id)) {
        return StatusOr<std::pair<std::string, std::vector<ArtifactMeta>>>::Fail(validationFail());
    }

    std::vector<ArtifactMeta> handles;
    if (context_obj->has("handles") && context_obj->isObject("handles")) {
        Poco::JSON::Object::Ptr handles_obj = context_obj->getObject("handles");
        for (const auto& key : handles_obj->getNames()) {
            const std::string path = "context.handles." + key;
            Poco::JSON::Object::Ptr entry;
            if (handles_obj->isObject(key)) {
                entry = handles_obj->getObject(key);
            }
            auto parsed = parseHandleEntry(key, entry, path);
            if (!parsed.ok()) {
                return StatusOr<std::pair<std::string, std::vector<ArtifactMeta>>>::Fail(
                    parsed.status());
            }
            handles.push_back(parsed.value());
        }
    }

    return StatusOr<std::pair<std::string, std::vector<ArtifactMeta>>>::Ok(
        std::make_pair(context_id, std::move(handles)));
}

} // namespace scheduler
} // namespace AIstudy
