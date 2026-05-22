#include "scheduler/skill_payload_validator.h"
#include "common/status/exception/error_category.h"
#include "common/status/exception/error_codes.h"
#include <Poco/JSON/Array.h>
#include <cmath>
#include <sstream>

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
    if (!schema->has("properties") || !schema->isObject("properties")) {
        return PayloadValidationResult::Success();
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
    return PayloadValidationResult::Success();
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
