#include "scheduler/skill_manifest.h"
#include "common/status/exception/error_category.h"
#include "common/status/exception/error_codes.h"
#include <Poco/File.h>
#include <Poco/JSON/Parser.h>
#include <fstream>
#include <sstream>

namespace AIstudy {
namespace scheduler {

namespace {

StatusOr<std::string> readTextFile(const std::string& path) {
    if (!Poco::File(path).exists()) {
        return StatusOr<std::string>::Fail(
            ErrorCodeWrapper(static_cast<int>(JsonError::FILE_READ_ERROR), ErrorCategory::JSON));
    }
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        return StatusOr<std::string>::Fail(
            ErrorCodeWrapper(static_cast<int>(JsonError::FILE_READ_ERROR), ErrorCategory::JSON));
    }
    std::ostringstream oss;
    oss << in.rdbuf();
    return StatusOr<std::string>::Ok(oss.str());
}

} // namespace

StatusOr<SkillManifest> loadSkillManifest(const std::string& manifest_path) {
    auto textRes = readTextFile(manifest_path);
    if (!textRes.ok()) {
        return StatusOr<SkillManifest>::Fail(textRes.status());
    }

    try {
        Poco::JSON::Parser parser;
        Poco::Dynamic::Var parsed = parser.parse(textRes.value());
        Poco::JSON::Object::Ptr root = parsed.extract<Poco::JSON::Object::Ptr>();
        if (!root || !root->has("id")) {
            return StatusOr<SkillManifest>::Fail(
                ErrorCodeWrapper(static_cast<int>(ValidationError::INVALID_INPUT),
                                  ErrorCategory::VALIDATION));
        }

        SkillManifest m;
        m.manifest_path = manifest_path;
        m.id = root->getValue<std::string>("id");
        if (root->has("version")) {
            m.version = root->getValue<std::string>("version");
        }
        if (root->has("title")) {
            m.title = root->getValue<std::string>("title");
        }
        if (root->has("description")) {
            m.description = root->getValue<std::string>("description");
        }
        if (root->has("timeout_ms")) {
            m.timeout_ms = root->getValue<int>("timeout_ms");
        }
        if (root->has("deprecated")) {
            m.deprecated = root->getValue<bool>("deprecated");
        }
        if (root->has("input_schema") && root->isObject("input_schema")) {
            m.input_schema = root->getObject("input_schema");
        }
        if (root->has("output_schema") && root->isObject("output_schema")) {
            m.output_schema = root->getObject("output_schema");
        }
        if (root->has("tags") && root->isArray("tags")) {
            auto arr = root->getArray("tags");
            for (size_t i = 0; i < arr->size(); ++i) {
                m.tags.push_back(arr->getElement<std::string>(static_cast<unsigned>(i)));
            }
        }
        return StatusOr<SkillManifest>::Ok(std::move(m));
    } catch (...) {
        return StatusOr<SkillManifest>::Fail(
            ErrorCodeWrapper(static_cast<int>(JsonError::PARSE_ERROR), ErrorCategory::JSON));
    }
}

Poco::JSON::Object::Ptr skillManifestToJson(const SkillManifest& manifest) {
    Poco::JSON::Object::Ptr obj(new Poco::JSON::Object);
    obj->set("id", manifest.id);
    obj->set("version", manifest.version);
    obj->set("title", manifest.title);
    obj->set("description", manifest.description);
    obj->set("timeout_ms", manifest.timeout_ms);
    obj->set("deprecated", manifest.deprecated);
    if (manifest.input_schema) {
        obj->set("input_schema", manifest.input_schema);
    }
    if (manifest.output_schema) {
        obj->set("output_schema", manifest.output_schema);
    }
    if (!manifest.tags.empty()) {
        Poco::JSON::Array::Ptr tags(new Poco::JSON::Array);
        for (const auto& t : manifest.tags) {
            tags->add(t);
        }
        obj->set("tags", tags);
    }
    return obj;
}

} // namespace scheduler
} // namespace AIstudy
