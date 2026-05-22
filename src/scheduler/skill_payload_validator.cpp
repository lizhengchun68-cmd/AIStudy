#include "scheduler/skill_payload_validator.h"
#include "common/status/exception/error_category.h"
#include "common/status/exception/error_codes.h"
#include <Poco/JSON/Array.h>
#include <cmath>
#include <unordered_set>

namespace AIstudy {
namespace scheduler {

PayloadValidationResult::PayloadValidationResult(bool success,
                                                 ErrorCodeWrapper error,
                                                 std::string detail)
    : success_(success), error_(std::move(error)), detail_(std::move(detail)) {}

PayloadValidationResult PayloadValidationResult::Success() {
    return PayloadValidationResult(true, ErrorCodeWrapper(), "");
}

PayloadValidationResult PayloadValidationResult::Fail(ErrorCodeWrapper error,
                                                    std::string detail) {
    return PayloadValidationResult(false, std::move(error), std::move(detail));
}

namespace {

enum class ValidationFailKind {
    MissingRequired,
    EnumMismatch,
    TypeMismatch,
    Constraint,
};

ErrorCodeWrapper validationCode(ValidationFailKind kind) {
    ValidationError code = ValidationError::INVALID_INPUT;
    switch (kind) {
    case ValidationFailKind::MissingRequired:
        code = ValidationError::INVALID_INPUT;
        break;
    case ValidationFailKind::EnumMismatch:
    case ValidationFailKind::Constraint:
        code = ValidationError::CONSTRAINT_VIOLATION;
        break;
    case ValidationFailKind::TypeMismatch:
        code = ValidationError::DATA_TYPE_MISMATCH;
        break;
    }
    return ErrorCodeWrapper(static_cast<int>(code), ErrorCategory::VALIDATION);
}

std::string formatDetail(const std::string& path, const std::string& reason) {
    if (path.empty()) {
        return reason;
    }
    return path + ": " + reason;
}

PayloadValidationResult failAt(const std::string& path,
                               const std::string& reason,
                               ValidationFailKind kind) {
    return PayloadValidationResult::Fail(validationCode(kind), formatDetail(path, reason));
}

PayloadValidationResult validateAgainstSchema(const Poco::Dynamic::Var& value,
                                              const Poco::JSON::Object::Ptr& schema,
                                              const std::string& path);

bool jsonTypeIs(const Poco::JSON::Object::Ptr& schema, const char* typeName) {
    return schema && schema->has("type")
        && schema->getValue<std::string>("type") == typeName;
}

bool additionalPropertiesForbidden(const Poco::JSON::Object::Ptr& schema) {
    return schema && schema->has("additionalProperties")
        && !schema->getValue<bool>("additionalProperties");
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

PayloadValidationResult validateAdditionalProperties(
    const Poco::JSON::Object::Ptr& obj,
    const Poco::JSON::Object::Ptr& schema,
    const Poco::JSON::Object::Ptr& props,
    const std::string& path) {
    if (!additionalPropertiesForbidden(schema)) {
        return PayloadValidationResult::Success();
    }
    std::unordered_set<std::string> allowed;
    if (props) {
        for (const auto& key : props->getNames()) {
            allowed.insert(key);
        }
    }
    for (const auto& key : obj->getNames()) {
        if (allowed.find(key) == allowed.end()) {
            return failAt(path.empty() ? key : path + "." + key,
                          "additional property not allowed",
                          ValidationFailKind::Constraint);
        }
    }
    return PayloadValidationResult::Success();
}

PayloadValidationResult validateArrayBounds(const Poco::JSON::Array::Ptr& arr,
                                            const Poco::JSON::Object::Ptr& schema,
                                            const std::string& path) {
    const size_t size = arr->size();
    if (schema->has("minItems")) {
        const auto minItems = static_cast<size_t>(schema->getValue<int>("minItems"));
        if (size < minItems) {
            return failAt(path, "array has too few items", ValidationFailKind::Constraint);
        }
    }
    if (schema->has("maxItems")) {
        const auto maxItems = static_cast<size_t>(schema->getValue<int>("maxItems"));
        if (size > maxItems) {
            return failAt(path, "array has too many items", ValidationFailKind::Constraint);
        }
    }
    return PayloadValidationResult::Success();
}

PayloadValidationResult validateString(const Poco::Dynamic::Var& value,
                                       const Poco::JSON::Object::Ptr& schema,
                                       const std::string& path) {
    if (!value.isString()) {
        return failAt(path, "expected string", ValidationFailKind::TypeMismatch);
    }
    const std::string s = value.convert<std::string>();
    if (schema->has("enum") && schema->isArray("enum")) {
        auto enums = schema->getArray("enum");
        for (size_t i = 0; i < enums->size(); ++i) {
            if (enums->getElement<std::string>(static_cast<unsigned>(i)) == s) {
                return PayloadValidationResult::Success();
            }
        }
        return failAt(path, "value not in enum", ValidationFailKind::EnumMismatch);
    }
    return PayloadValidationResult::Success();
}

PayloadValidationResult validateNumberLike(const Poco::Dynamic::Var& value,
                                           const Poco::JSON::Object::Ptr& schema,
                                           const std::string& path,
                                           bool requireInteger) {
    if (requireInteger) {
        if (!valueIsInteger(value)) {
            return failAt(path, "expected integer", ValidationFailKind::TypeMismatch);
        }
    } else if (!value.isNumeric()) {
        return failAt(path, "expected number", ValidationFailKind::TypeMismatch);
    }
    const double n = value.convert<double>();
    if (schema->has("minimum")) {
        const double minv = schema->getValue<double>("minimum");
        if (n < minv) {
            return failAt(path, "below minimum", ValidationFailKind::Constraint);
        }
    }
    if (schema->has("maximum")) {
        const double maxv = schema->getValue<double>("maximum");
        if (n > maxv) {
            return failAt(path, "above maximum", ValidationFailKind::Constraint);
        }
    }
    return PayloadValidationResult::Success();
}

PayloadValidationResult validateArray(const Poco::Dynamic::Var& value,
                                      const Poco::JSON::Object::Ptr& schema,
                                      const std::string& path) {
    Poco::JSON::Array::Ptr arr = value.extract<Poco::JSON::Array::Ptr>();
    if (!arr) {
        return failAt(path, "expected array", ValidationFailKind::TypeMismatch);
    }
    auto bounds = validateArrayBounds(arr, schema, path);
    if (!bounds.ok()) {
        return bounds;
    }
    if (!schema->has("items") || !schema->isObject("items")) {
        return PayloadValidationResult::Success();
    }
    Poco::JSON::Object::Ptr itemSchema = schema->getObject("items");
    for (size_t i = 0; i < arr->size(); ++i) {
        const std::string elemPath = path + "[" + std::to_string(i) + "]";
        auto r = validateAgainstSchema(arr->get(static_cast<unsigned>(i)), itemSchema, elemPath);
        if (!r.ok()) {
            return r;
        }
    }
    return PayloadValidationResult::Success();
}

PayloadValidationResult validateObject(const Poco::Dynamic::Var& value,
                                       const Poco::JSON::Object::Ptr& schema,
                                       const std::string& path) {
    Poco::JSON::Object::Ptr obj = value.extract<Poco::JSON::Object::Ptr>();
    if (!obj) {
        return failAt(path, "expected object", ValidationFailKind::TypeMismatch);
    }
    Poco::JSON::Object::Ptr props;
    if (schema->has("properties") && schema->isObject("properties")) {
        props = schema->getObject("properties");
    }
    if (schema->has("required") && schema->isArray("required")) {
        auto required = schema->getArray("required");
        for (size_t i = 0; i < required->size(); ++i) {
            const std::string key = required->getElement<std::string>(static_cast<unsigned>(i));
            if (!obj->has(key)) {
                return failAt(path.empty() ? key : path + "." + key,
                              "required field missing",
                              ValidationFailKind::MissingRequired);
            }
        }
    }
    if (props) {
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
    }
    return validateAdditionalProperties(obj, schema, props, path);
}

PayloadValidationResult validateAgainstSchema(const Poco::Dynamic::Var& value,
                                              const Poco::JSON::Object::Ptr& schema,
                                              const std::string& path) {
    if (!schema) {
        return PayloadValidationResult::Success();
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
            return failAt(path, "expected boolean", ValidationFailKind::TypeMismatch);
        }
        return PayloadValidationResult::Success();
    }
    return PayloadValidationResult::Success();
}

} // namespace

PayloadValidationResult validatePayloadAgainstManifest(const Poco::JSON::Object::Ptr& payload,
                                                       const SkillManifest& manifest) {
    if (!payload) {
        return PayloadValidationResult::Fail(
            validationCode(ValidationFailKind::MissingRequired), "payload is null or missing");
    }
    if (!manifest.input_schema) {
        return PayloadValidationResult::Success();
    }
    return validateObject(Poco::Dynamic::Var(payload), manifest.input_schema, "");
}

} // namespace scheduler
} // namespace AIstudy
