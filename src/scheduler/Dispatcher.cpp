#include "scheduler/Dispatcher.h"

#include "common/logger/logger.h"
#include "common/status/api_response.h"
#include "common/status/exception/error_category.h"
#include "common/status/exception/error_codes.h"
#include "scheduler/skill_payload_validator.h"
#include "scheduler/skill_protocol.h"
#include "scheduler/skill_context_store.h"
#include "scheduler/skill_execution_context.h"
#include "scheduler/skill_registry.h"
#include "scheduler/context_store_errors.h"
#include <Poco/Format.h>
#include <Poco/JSON/Array.h>
#include <Poco/JSON/Object.h>
#include <Poco/JSON/Parser.h>
#include <Poco/JSON/Stringifier.h>
#include <chrono>
#include <memory>
#include <sstream>

namespace AIstudy {
namespace scheduler {
namespace {

void logExecuteBegin(const std::string& request_id,
                   const std::string& skill_id,
                   int timeout_ms,
                   const std::string& context_id) {
    auto log = common::logger::Logger::get("AIstudy.Dispatcher");
    if (context_id.empty()) {
        log.info(Poco::format("execute begin request_id=%s skill_id=%s", request_id, skill_id));
    } else {
        log.info(Poco::format("execute begin request_id=%s skill_id=%s context_id=%s",
                              request_id,
                              skill_id,
                              context_id));
    }
    if (timeout_ms > 0) {
        log.info(Poco::format(
            "options.timeout_ms=%d (reserved: not enforced; logged only)", timeout_ms));
    }
}

void logExecuteEnd(const std::string& request_id,
                   const std::string& skill_id,
                   int duration_ms,
                   bool ok,
                   int error_code,
                   const std::string& error_category) {
    common::logger::Logger::get("AIstudy.Dispatcher").info(Poco::format(
        "execute end request_id=%s skill_id=%s duration_ms=%d ok=%s error_code=%d category=%s",
        request_id,
        skill_id,
        duration_ms,
        ok ? std::string("true") : std::string("false"),
        error_code,
        error_category));
}

struct ActiveContextScope {
    ~ActiveContextScope() { skill_execution_context::clearActiveContext(); }
};

int parseResponseSummary(const std::string& response_json, bool* ok_out, int* error_code_out,
                         std::string* error_category_out) {
    *ok_out = false;
    *error_code_out = 0;
    if (error_category_out) {
        error_category_out->clear();
    }
    try {
        Poco::JSON::Parser parser;
        Poco::JSON::Object::Ptr root =
            parser.parse(response_json).extract<Poco::JSON::Object::Ptr>();
        if (!root || !root->has("ok")) {
            return 0;
        }
        *ok_out = root->getValue<bool>("ok");
        if (!*ok_out && root->has("error") && root->isObject("error")) {
            Poco::JSON::Object::Ptr err = root->getObject("error");
            if (err->has("code")) {
                *error_code_out = err->getValue<int>("code");
            }
            if (error_category_out && err->has("category")) {
                *error_category_out = err->getValue<std::string>("category");
            }
        }
    } catch (...) {
    }
    return 0;
}

} // namespace

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
    const auto wall_start = std::chrono::steady_clock::now();
    ContextStore::instance().purgeExpiredContexts();

    const auto envelopeRes = parseSkillEnvelope(envelope_json);
    if (!envelopeRes.ok()) {
        const std::string detail = lastEnvelopeValidationDetail();
        const std::string response = makeSkillApiFailureResponse(
            envelopeRes.status(), "", ApiResponseMeta{}, detail);
        const int duration_ms = static_cast<int>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - wall_start)
                .count());
        logExecuteEnd("", "", duration_ms, false, envelopeRes.status().code(),
                      envelopeRes.status().category());
        return response;
    }

    const SkillEnvelope& envelope = envelopeRes.value();
    logExecuteBegin(envelope.request_id, envelope.skill_id, envelope.timeout_ms, envelope.context_id);

    std::unique_ptr<ActiveContextScope> context_scope;
    if (!envelope.context_id.empty()) {
        auto& store = ContextStore::instance();
        const auto ensured = store.ensureContext(envelope.context_id);
        if (!ensured.ok()) {
            const std::string& detail = lastContextStoreDetail();
            const std::string response = makeSkillApiFailureResponse(
                ensured.status(), envelope.request_id, ApiResponseMeta{}, detail);
            const int duration_ms = static_cast<int>(
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - wall_start)
                    .count());
            logExecuteEnd(envelope.request_id, envelope.skill_id, duration_ms, false,
                          ensured.status().code(), ensured.status().category());
            return response;
        }
        if (!envelope.inbound_handles.empty()) {
            const auto merged = store.mergeInbound(envelope.context_id, envelope.inbound_handles);
            if (!merged.ok()) {
                const std::string& detail = lastContextStoreDetail();
                const std::string response = makeSkillApiFailureResponse(
                    merged.status(), envelope.request_id, ApiResponseMeta{}, detail);
                const int duration_ms = static_cast<int>(
                    std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now() - wall_start)
                        .count());
                logExecuteEnd(envelope.request_id, envelope.skill_id, duration_ms, false,
                              merged.status().code(), merged.status().category());
                return response;
            }
        }
        skill_execution_context::setActiveContext(envelope.context_id);
        context_scope = std::make_unique<ActiveContextScope>();
    }

    const SkillEntry* entry = findSkill(envelope.skill_id);
    if (!entry) {
        const ErrorCodeWrapper err(static_cast<int>(SystemError::UNKNOWN_ERROR),
                                   ErrorCategory::SYSTEM);
        const std::string response = makeSkillApiFailureResponse(
            err, envelope.request_id, ApiResponseMeta{},
            "Unknown skill_id: " + envelope.skill_id);
        const int duration_ms = static_cast<int>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - wall_start)
                .count());
        logExecuteEnd(envelope.request_id, envelope.skill_id, duration_ms, false, err.code(),
                      err.category());
        return response;
    }

    ApiResponseMeta meta;
    meta.skill_id = envelope.skill_id;
    meta.skill_version = entry->manifest.version;

    if (!envelope.skill_version.empty()
        && envelope.skill_version != entry->manifest.version) {
        const ErrorCodeWrapper err(static_cast<int>(ValidationError::INVALID_INPUT),
                                   ErrorCategory::VALIDATION);
        const std::string response = makeSkillApiFailureResponse(
            err, envelope.request_id, meta, "skill_version mismatch");
        const int duration_ms = static_cast<int>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - wall_start)
                .count());
        logExecuteEnd(envelope.request_id, envelope.skill_id, duration_ms, false, err.code(),
                      err.category());
        return response;
    }

    const auto validRes = validatePayloadAgainstManifest(envelope.payload, entry->manifest);
    if (!validRes.ok()) {
        const std::string response = makeSkillApiFailureResponse(
            validRes.error(), envelope.request_id, meta, validRes.detail());
        const int duration_ms = static_cast<int>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - wall_start)
                .count());
        logExecuteEnd(envelope.request_id, envelope.skill_id, duration_ms, false,
                      validRes.error().code(), validRes.error().category());
        return response;
    }

    std::ostringstream payload_oss;
    Poco::JSON::Stringifier::condense(envelope.payload, payload_oss);
    const std::string payload_str = payload_oss.str();

    const auto t0 = std::chrono::steady_clock::now();
    const StatusOr<SkillResultJson> skill_result = entry->func(payload_str);
    const auto t1 = std::chrono::steady_clock::now();
    meta.duration_ms = static_cast<int>(
        std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count());

    std::string response;
    if (!skill_result.ok() && !lastContextStoreDetail().empty()) {
        response = makeSkillApiFailureResponse(
            skill_result.status(), envelope.request_id, meta, lastContextStoreDetail());
    } else {
        response = makeSkillApiResponse(skill_result, envelope.request_id, meta);
    }

    bool ok = false;
    int error_code = 0;
    std::string error_category;
    parseResponseSummary(response, &ok, &error_code, &error_category);
    if (!skill_result.ok() && error_code == 0) {
        error_code = skill_result.status().code();
        error_category = skill_result.status().category();
    }

    const int duration_ms = static_cast<int>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - wall_start)
            .count());
    logExecuteEnd(envelope.request_id, envelope.skill_id, duration_ms, ok, error_code,
                  error_category);

    context_scope.reset();
    if (envelope.context_close && !envelope.context_id.empty()) {
        ContextStore::instance().closeContext(envelope.context_id);
    }

    return response;
}

std::string Dispatcher::healthJson() const {
    Poco::JSON::Object::Ptr root(new Poco::JSON::Object);
    root->set("protocol", "1");
    const bool skills_ok = !registry_.empty();
    root->set("ok", skills_ok);
    root->set("project_root", projectRootPath());
    root->set("skills_loaded", static_cast<int>(registry_.size()));
    root->set("skills_load_failed", static_cast<int>(load_failures_.size()));

    Poco::JSON::Object::Ptr checks(new Poco::JSON::Object);
    checks->set("poco", true);
#ifdef AISTUDY_HDF5_ENABLED
    checks->set("hdf5", true);
#else
    checks->set("hdf5", false);
#endif
    root->set("checks", checks);

    std::ostringstream oss;
    Poco::JSON::Stringifier::condense(root, oss);
    return oss.str();
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
