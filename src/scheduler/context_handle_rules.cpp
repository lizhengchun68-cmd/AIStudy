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

bool isAllowedIdChar(char c) {
    return std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_' || c == '.' || c == '-';
}

} // namespace

std::string formatValidationDetail(const std::string& path, const std::string& reason) {
    if (path.empty()) {
        return reason;
    }
    return path + ": " + reason;
}

std::string describeInvalidContextId(const std::string& context_id) {
    if (context_id.empty()) {
        return formatValidationDetail("context.context_id", "must not be empty");
    }
    if (context_id.size() > 64) {
        return formatValidationDetail("context.context_id", "exceeds max length 64");
    }
    for (char c : context_id) {
        if (!isAllowedIdChar(c)) {
            return formatValidationDetail("context.context_id",
                                          "invalid character (allowed: alnum, _, ., -)");
        }
    }
    return formatValidationDetail("context.context_id", "invalid context_id");
}

std::string describeInvalidHandleId(const std::string& handle_id) {
    const std::string path =
        handle_id.empty() ? "context.handles" : ("context.handles." + handle_id);
    if (handle_id.empty()) {
        return formatValidationDetail(path, "handle_id must not be empty");
    }
    if (handle_id.size() > 128) {
        return formatValidationDetail(path, "exceeds max length 128");
    }
    if (handleKindFromPrefix(handle_id) == HandleKind::Unknown) {
        return formatValidationDetail(path, "invalid handle_id prefix (expected mesh_/result_/file_)");
    }
    for (char c : handle_id) {
        if (!isAllowedIdChar(c)) {
            return formatValidationDetail(path,
                                          "invalid character (allowed: alnum, _, ., -)");
        }
    }
    return formatValidationDetail(path, "invalid handle_id");
}

ContextParseResult::ContextParseResult(bool success,
                                       std::string context_id,
                                       std::vector<ArtifactMeta> inbound,
                                       ErrorCodeWrapper error,
                                       std::string detail)
    : success_(success),
      context_id_(std::move(context_id)),
      inbound_handles_(std::move(inbound)),
      error_(std::move(error)),
      detail_(std::move(detail)) {}

ContextParseResult ContextParseResult::success(std::string context_id,
                                             std::vector<ArtifactMeta> inbound,
                                             bool context_close) {
    ContextParseResult r(true, std::move(context_id), std::move(inbound), ErrorCodeWrapper(), "");
    r.context_close_ = context_close;
    return r;
}

ContextParseResult ContextParseResult::failure(const std::string& detail) {
    return ContextParseResult(false, "", {}, validationFail(), detail);
}

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
        return StatusOr<ArtifactMeta>::Fail(validationFail());
    }
    ArtifactMeta meta;
    meta.handle_id = handle_id;
    meta.kind = handleKindFromPrefix(handle_id);

    if (obj) {
        if (obj->has("kind")) {
            const std::string kind_str = obj->getValue<std::string>("kind");
            const HandleKind declared = parseKindString(kind_str);
            if (declared == HandleKind::Unknown) {
                return StatusOr<ArtifactMeta>::Fail(validationFail());
            }
            if (!handleKindMatchesId(handle_id, declared)) {
                return StatusOr<ArtifactMeta>::Fail(validationFail());
            }
            meta.kind = declared;
        }
        if (obj->has("uri")) {
            meta.uri = obj->getValue<std::string>("uri");
        }
    }
    return StatusOr<ArtifactMeta>::Ok(std::move(meta));
}

ContextParseResult parseContextObjectDetailed(const Poco::JSON::Object::Ptr& context_obj) {
    if (!context_obj) {
        return ContextParseResult::success("", {});
    }

    if (!context_obj->has("context_id")) {
        return ContextParseResult::failure(
            formatValidationDetail("context", "context_id required"));
    }

    const std::string context_id = context_obj->getValue<std::string>("context_id");
    if (!isValidContextId(context_id)) {
        return ContextParseResult::failure(describeInvalidContextId(context_id));
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
            if (!isValidHandleId(key)) {
                return ContextParseResult::failure(describeInvalidHandleId(key));
            }
            auto parsed = parseHandleEntry(key, entry, path);
            if (!parsed.ok()) {
                if (entry && entry->has("kind")) {
                    const HandleKind declared = parseKindString(entry->getValue<std::string>("kind"));
                    if (declared == HandleKind::Unknown) {
                        return ContextParseResult::failure(
                            formatValidationDetail(path, "unknown kind"));
                    }
                    return ContextParseResult::failure(formatValidationDetail(
                        path, "kind does not match handle_id prefix"));
                }
                return ContextParseResult::failure(
                    formatValidationDetail(path, "invalid handle entry"));
            }
            handles.push_back(parsed.value());
        }
    }

    bool close_session = false;
    if (context_obj->has("close")) {
        if (!context_obj->get("close").isBoolean()) {
            return ContextParseResult::failure(
                formatValidationDetail("context.close", "must be boolean"));
        }
        close_session = context_obj->getValue<bool>("close");
    }

    return ContextParseResult::success(context_id, std::move(handles), close_session);
}

StatusOr<std::pair<std::string, std::vector<ArtifactMeta>>> parseContextObject(
    const Poco::JSON::Object::Ptr& context_obj) {
    const auto detailed = parseContextObjectDetailed(context_obj);
    if (!detailed.ok()) {
        return StatusOr<std::pair<std::string, std::vector<ArtifactMeta>>>::Fail(detailed.error());
    }
    return StatusOr<std::pair<std::string, std::vector<ArtifactMeta>>>::Ok(
        std::make_pair(detailed.context_id(), detailed.inbound_handles()));
}

} // namespace scheduler
} // namespace AIstudy
