#include "scheduler/skill_payload_validator.h"
#include "common/status/exception/error_category.h"
#include "common/status/exception/error_codes.h"
#include <Poco/JSON/Array.h>
#include <cmath>
#include <sstream>

namespace AIstudy {
namespace scheduler {
namespace {

ErrorCodeWrapper validationError() {
    return ErrorCodeWrapper(static_cast<int>(ValidationError::INVALID_INPUT),
                          ErrorCategory::VALIDATION);
}

StatusOr<bool> failAt(const std::string& /*path*/, const std::string& /*detail*/) {
    return StatusOr<bool>::Fail(validationError());
}

bool jsonTypeIs(const Poco::JSON::Object::Ptr& schema, const char* typeName) {
    return schema && schema->has("type")
        && schema->getValue<std::string>("type") == typeName;
}

bool valueIsInteger(const Poco::Dynamic::Var& v) {
    if (v.isInteger()) {
        return true;
    }
    if (v.isNumeric()) {
        const double d = v.convert<double>();
        return std::floor(d) == d;
    }
    return false;
}

StatusOr<bool> validateAgainstSchema(const Poco::Dynamic::Var& value,
                                     const Poco::JSON::Object::Ptr& schema,
                                     const std::string& path);

StatusOr<bool> validateString(const Poco::Dynamic::Var& value,
                              const Poco::JSON::Object::Ptr& schema,
                              const std::string& path) {
    if (!value.isString()) {
        return failAt(path, "expected string");
    }
    const std::string s = value.convert<std::string>();
    if (schema->has("enum") && schema->isArray("enum")) {
        auto enums = schema->getArray("enum");
        for (size_t i = 0; i < enums->size(); ++i) {
            if (enums->getElement<std::string>(static_cast<unsigned>(i)) == s) {
                return StatusOr<bool>::Ok(true);
            }
        }
        return failAt(path, "value not in enum");
    }
    return StatusOr<bool>::Ok(true);
}

StatusOr<bool> validateNumberLike(const Poco::Dynamic::Var& value,
                                  const Poco::JSON::Object::Ptr& schema,
                                  const std::string& path,
                                  bool requireInteger) {
    if (requireInteger) {
        if (!valueIsInteger(value)) {
            return failAt(path, "expected integer");
        }
    } else if (!value.isNumeric()) {
        return failAt(path, "expected number");
    }
    const double n = value.convert<double>();
    if (schema->has("minimum")) {
        const double minv = schema->getValue<double>("minimum");
        if (n < minv) {
            return failAt(path, "below minimum");
        }
    }
    if (schema->has("maximum")) {
        const double maxv = schema->getValue<double>("maximum");
        if (n > maxv) {
            return failAt(path, "above maximum");
        }
    }
    return StatusOr<bool>::Ok(true);
}

StatusOr<bool> validateArray(const Poco::Dynamic::Var& value,
                            const Poco::JSON::Object::Ptr& schema,
                            const std::string& path) {
    Poco::JSON::Array::Ptr arr = value.extract<Poco::JSON::Array::Ptr>();
    if (!arr) {
        return failAt(path, "expected array");
    }
    if (!schema->has("items") || !schema->isObject("items")) {
        return StatusOr<bool>::Ok(true);
    }
    Poco::JSON::Object::Ptr itemSchema = schema->getObject("items");
    for (size_t i = 0; i < arr->size(); ++i) {
        const std::string elemPath = path + "[" + std::to_string(i) + "]";
        auto r = validateAgainstSchema(arr->get(static_cast<unsigned>(i)), itemSchema, elemPath);
        if (!r.ok()) {
            return r;
        }
    }
    return StatusOr<bool>::Ok(true);
}

StatusOr<bool> validateObject(const Poco::Dynamic::Var& value,
                             const Poco::JSON::Object::Ptr& schema,
                             const std::string& path) {
    Poco::JSON::Object::Ptr obj = value.extract<Poco::JSON::Object::Ptr>();
    if (!obj) {
        return failAt(path, "expected object");
    }
    if (schema->has("required") && schema->isArray("required")) {
        auto required = schema->getArray("required");
        for (size_t i = 0; i < required->size(); ++i) {
            const std::string key = required->getElement<std::string>(static_cast<unsigned>(i));
            if (!obj->has(key)) {
                return failAt(path.empty() ? key : path + "." + key, "required field missing");
            }
        }
    }
    if (!schema->has("properties") || !schema->isObject("properties")) {
        return StatusOr<bool>::Ok(true);
    }
    Poco::JSON::Object::Ptr props = schema->getObject("properties");
    for (const auto& key : props->getNames()) {
        if (!obj->has(key)) {
            continue;
        }
        const std::string childPath = path.empty() ? key : path + "." + key;
        if (!props->isObject(key)) {
            continue;
        }
        auto r = validateAgainstSchema(obj->get(key), props->getObject(key), childPath);
        if (!r.ok()) {
            return r;
        }
    }
    return StatusOr<bool>::Ok(true);
}

StatusOr<bool> validateAgainstSchema(const Poco::Dynamic::Var& value,
                                     const Poco::JSON::Object::Ptr& schema,
                                     const std::string& path) {
    if (!schema) {
        return StatusOr<bool>::Ok(true);
    }
    if (jsonTypeIs(schema, "object")) {
        return validateObject(value, schema, path);
    }
    if (jsonTypeIs(schema, "array")) {
        return validateArray(value, schema, path);
    }
    if (jsonTypeIs(schema, "string")) {
        return validateString(value, schema, path);
    }
    if (jsonTypeIs(schema, "integer")) {
        return validateNumberLike(value, schema, path, true);
    }
    if (jsonTypeIs(schema, "number")) {
        return validateNumberLike(value, schema, path, false);
    }
    if (jsonTypeIs(schema, "boolean")) {
        if (!value.isBoolean()) {
            return failAt(path, "expected boolean");
        }
        return StatusOr<bool>::Ok(true);
    }
    return StatusOr<bool>::Ok(true);
}

} // namespace

StatusOr<bool> validatePayloadAgainstManifest(const Poco::JSON::Object::Ptr& payload,
                                                const SkillManifest& manifest) {
    if (!payload) {
        return StatusOr<bool>::Fail(validationError());
    }
    if (!manifest.input_schema) {
        return StatusOr<bool>::Ok(true);
    }
    return validateObject(Poco::Dynamic::Var(payload), manifest.input_schema, "");
}

} // namespace scheduler
} // namespace AIstudy
