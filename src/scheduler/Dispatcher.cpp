#include "scheduler/Dispatcher.h"

#include "common/status/api_response.h"
#include "common/status/exception/error_category.h"
#include "common/status/exception/error_codes.h"
#include "scheduler/skill_payload_validator.h"
#include "scheduler/skill_protocol.h"
#include <Poco/JSON/Array.h>
#include <Poco/JSON/Object.h>
#include <Poco/JSON/Stringifier.h>
#include <chrono>
#include <sstream>

namespace AIstudy {
namespace scheduler {

void Dispatcher::recordLoadFailure(SkillLoadFailure failure) {
    load_failures_.push_back(std::move(failure));
}

void Dispatcher::registerSkill(const SkillManifest& manifest, SkillExecuteFunc func) {
    SkillEntry entry;
    entry.func = func;
    entry.manifest = manifest;
    registry_[manifest.id] = std::move(entry);
}

const Dispatcher::SkillEntry* Dispatcher::findSkill(const std::string& skill_id) const {
    const auto it = registry_.find(skill_id);
    if (it == registry_.end()) {
        return nullptr;
    }
    return &it->second;
}

std::string Dispatcher::execute(const std::string& envelope_json) {
    const auto envelopeRes = parseSkillEnvelope(envelope_json);
    if (!envelopeRes.ok()) {
        return makeSkillApiFailureResponse(envelopeRes.status(), "", ApiResponseMeta{});
    }

    const SkillEnvelope& envelope = envelopeRes.value();
    const SkillEntry* entry = findSkill(envelope.skill_id);
    if (!entry) {
        const ErrorCodeWrapper err(static_cast<int>(SystemError::UNKNOWN_ERROR),
                                   ErrorCategory::SYSTEM);
        return makeSkillApiFailureResponse(
            err, envelope.request_id, ApiResponseMeta{},
            "Unknown skill_id: " + envelope.skill_id);
    }

    ApiResponseMeta meta;
    meta.skill_id = envelope.skill_id;
    meta.skill_version = entry->manifest.version;

    if (!envelope.skill_version.empty()
        && envelope.skill_version != entry->manifest.version) {
        const ErrorCodeWrapper err(static_cast<int>(ValidationError::INVALID_INPUT),
                                   ErrorCategory::VALIDATION);
        return makeSkillApiFailureResponse(
            err, envelope.request_id, meta, "skill_version mismatch");
    }

    const auto validRes = validatePayloadAgainstManifest(envelope.payload, entry->manifest);
    if (!validRes.ok()) {
        return makeSkillApiFailureResponse(validRes.status(), envelope.request_id, meta);
    }

    std::ostringstream payload_oss;
    Poco::JSON::Stringifier::condense(envelope.payload, payload_oss);
    const std::string payload_str = payload_oss.str();

    const auto t0 = std::chrono::steady_clock::now();
    const StatusOr<SkillResultJson> skill_result = entry->func(payload_str);
    const auto t1 = std::chrono::steady_clock::now();
    meta.duration_ms = static_cast<int>(
        std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count());

    return makeSkillApiResponse(skill_result, envelope.request_id, meta);
}

std::string Dispatcher::listSkillsJson() const {
    Poco::JSON::Object::Ptr root(new Poco::JSON::Object);
    Poco::JSON::Array::Ptr skills(new Poco::JSON::Array);
    for (const auto& pair : registry_) {
        const SkillEntry& entry = pair.second;
        Poco::JSON::Object::Ptr item(new Poco::JSON::Object);
        item->set("id", pair.first);
        item->set("version", entry.manifest.version);
        item->set("title", entry.manifest.title);
        item->set("description", entry.manifest.description);
        item->set("deprecated", entry.manifest.deprecated);
        skills->add(item);
    }
    root->set("skills", skills);
    if (!load_failures_.empty()) {
        Poco::JSON::Array::Ptr errors(new Poco::JSON::Array);
        for (const auto& f : load_failures_) {
            Poco::JSON::Object::Ptr item(new Poco::JSON::Object);
            item->set("id", f.binding_id);
            item->set("manifest_path", f.manifest_path);
            item->set("load_error", f.error);
            errors->add(item);
        }
        root->set("load_errors", errors);
    }
    std::ostringstream oss;
    Poco::JSON::Stringifier::condense(root, oss);
    return oss.str();
}

std::string Dispatcher::describeSkillJson(const std::string& skill_id) const {
    const SkillEntry* entry = findSkill(skill_id);
    if (!entry) {
        Poco::JSON::Object::Ptr err(new Poco::JSON::Object);
        err->set("ok", false);
        err->set("error", "Unknown skill_id: " + skill_id);
        std::ostringstream oss;
        Poco::JSON::Stringifier::condense(err, oss);
        return oss.str();
    }
    std::ostringstream oss;
    Poco::JSON::Stringifier::condense(skillManifestToJson(entry->manifest), oss);
    return oss.str();
}

} // namespace scheduler
} // namespace AIstudy
