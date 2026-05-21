#ifndef COMMON_IO_JSON_JSON_SERIALIZABLE_H
#define COMMON_IO_JSON_JSON_SERIALIZABLE_H

#include "common/status/status_or.h"
#include "common/status/exception/error_codes.h"
#include "common/status/exception/error_category.h"
#include <Poco/JSON/Object.h>

namespace AIstudy {
namespace common {
namespace io {
namespace json {


/**
 * @brief Serialize T to JSON object. T must provide ToJson() const -> Object::Ptr.
 */
template <typename T>
Poco::JSON::Object::Ptr serializeToJson(const T& t) {
    return t.ToJson();
}

/**
 * @brief Deserialize T from JSON object. T must provide static FromJson(Object::Ptr) -> T.
 *        FromJson may throw; use deserializeFromJsonStatusOr for StatusOr.
 */
template <typename T>
T deserializeFromJson(const Poco::JSON::Object::Ptr& o) {
    return T::FromJson(o);
}

/**
 * @brief Deserialize to StatusOr<T>. Catches exceptions and returns Fail(JsonError::TYPE_MISMATCH).
 */
template <typename T>
StatusOr<T> deserializeFromJsonStatusOr(const Poco::JSON::Object::Ptr& o) {
    if (!o) {
        return StatusOr<T>::Fail(
            ErrorCodeWrapper(static_cast<int>(JsonError::TYPE_MISMATCH), ErrorCategory::JSON));
    }
    try {
        return StatusOr<T>::Ok(T::FromJson(o));
    } catch (...) {
        return StatusOr<T>::Fail(
            ErrorCodeWrapper(static_cast<int>(JsonError::TYPE_MISMATCH), ErrorCategory::JSON));
    }
}

} // namespace json
} // namespace io
} // namespace common
} // namespace AIstudy


#endif // COMMON_IO_JSON_JSON_SERIALIZABLE_H
