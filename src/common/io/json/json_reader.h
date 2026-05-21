#ifndef COMMON_IO_JSON_JSON_READER_H
#define COMMON_IO_JSON_JSON_READER_H

#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <initializer_list>
#include <istream>
#include <functional>
#include "common/status/exception/error_code_wrapper.h"
#include "common/status/exception/error_codes.h"
#include "common/status/status_or.h"
#include "common/io/json/json_exception.h"
#include "common/io/json/json_io_options.h"
#include "common/status/exception/error_category.h"
#include <Poco/Dynamic/Var.h>
#include <Poco/JSON/Object.h>
#include <Poco/JSON/Array.h>
#include <Poco/JSON/Query.h>
#include <Poco/Nullable.h>

namespace AIstudy {
namespace common {
namespace io {
namespace json {


/**
 * @brief JSON reader (straightforward impl, no factory/PIMPL)
 */
class JsonReader {
public:
    JsonReader() = default;

    StatusOr<bool> readFile(const std::string& filepath);
    StatusOr<bool> readString(const std::string& json);
    StatusOr<bool> readStream(std::istream& in);

    template <typename T>
    StatusOr<T> getValue(const std::string& path) const;

    /** @brief Like getValue<T> but key missing or null �?default; type mismatch still throws. */
    template <typename T>
    T getValue(const std::string& path, const T& defaultVal) const;

    StatusOr<std::string> getString(const std::string& path) const;
    StatusOr<int> getInt(const std::string& path) const;
    StatusOr<double> getDouble(const std::string& path) const;
    StatusOr<bool> getBool(const std::string& path) const;

    std::string getString(const std::string& path, const std::string& defaultVal) const;
    int getInt(const std::string& path, int defaultVal) const;
    double getDouble(const std::string& path, double defaultVal) const;
    bool getBool(const std::string& path, bool defaultVal) const;

    StatusOr<Poco::JSON::Object::Ptr> getObject(const std::string& path) const;
    StatusOr<Poco::JSON::Array::Ptr> getArray(const std::string& path) const;

    template <typename T>
    StatusOr<std::vector<T>> getVector(const std::string& path) const;
    template <typename T>
    StatusOr<std::map<std::string, T>> getMap(const std::string& path) const;
    template <typename T>
    StatusOr<std::unordered_map<std::string, T>> getUnorderedMap(const std::string& path) const;

    /** @brief Optional value: missing or null �?Ok(Nullable{}); type mismatch �?Fail. Uses Poco::Nullable (optional-like). */
    template <typename T>
    StatusOr<Poco::Nullable<T>> getValueOptional(const std::string& path) const;

    StatusOr<bool> checkRequiredKeys(std::initializer_list<std::string> paths) const;

    bool hasKey(const std::string& path) const;
    /** @brief True if value at path is missing or JSON null (isEmpty). */
    bool isNull(const std::string& path) const;
    StatusOr<std::vector<std::string>> getKeys(const std::string& path) const;

    bool isLoaded() const;

    /** @brief Buffer size for readFile (default 8K). §4 大文件缓�?*/
    void setBufferSize(std::size_t n) { bufferSize_ = n; }
    std::size_t getBufferSize() const { return bufferSize_; }
    /** @brief Last parse error message (Poco detail, possibly line/col). Cleared on success. */
    const std::string& getLastParseError() const { return lastParseError_; }

private:
    bool parseImpl(std::function<Poco::Dynamic::Var()> fn, ErrorCodeWrapper* err, bool isFile, const std::string& pathOrMsg);

    Poco::Dynamic::Var root_;
    std::size_t bufferSize_ = kJsonDefaultBufferSize;
    mutable std::string lastParseError_;
    std::vector<char> readBuffer_;
};

template <typename T>
StatusOr<T> JsonReader::getValue(const std::string& path) const {
    Poco::JSON::Query q(root_);
    Poco::Dynamic::Var v = q.find(path);
    if (v.isEmpty())
        return StatusOr<T>::Fail(ErrorCodeWrapper(static_cast<int>(JsonError::KEY_NOT_FOUND), ErrorCategory::JSON));
    try {
        return StatusOr<T>::Ok(v.convert<T>());
    } catch (...) {
        return StatusOr<T>::Fail(ErrorCodeWrapper(static_cast<int>(JsonError::TYPE_MISMATCH), ErrorCategory::JSON));
    }
}

template <typename T>
T JsonReader::getValue(const std::string& path, const T& defaultVal) const {
    Poco::JSON::Query q(root_);
    Poco::Dynamic::Var v = q.find(path);
    if (v.isEmpty())
        return defaultVal;
    try {
        return v.convert<T>();
    } catch (...) {
        throw JsonParseException("type mismatch at path: " + path,
            ErrorCodeWrapper(static_cast<int>(JsonError::TYPE_MISMATCH), ErrorCategory::JSON));
    }
}

inline StatusOr<std::string> JsonReader::getString(const std::string& path) const { return getValue<std::string>(path); }
inline StatusOr<int> JsonReader::getInt(const std::string& path) const { return getValue<int>(path); }
inline StatusOr<double> JsonReader::getDouble(const std::string& path) const { return getValue<double>(path); }
inline StatusOr<bool> JsonReader::getBool(const std::string& path) const { return getValue<bool>(path); }

inline std::string JsonReader::getString(const std::string& path, const std::string& defaultVal) const {
    return getValue<std::string>(path, defaultVal);
}
inline int JsonReader::getInt(const std::string& path, int defaultVal) const { return getValue<int>(path, defaultVal); }
inline double JsonReader::getDouble(const std::string& path, double defaultVal) const {
    return getValue<double>(path, defaultVal);
}
inline bool JsonReader::getBool(const std::string& path, bool defaultVal) const {
    return getValue<bool>(path, defaultVal);
}

inline bool JsonReader::isNull(const std::string& path) const {
    Poco::JSON::Query q(root_);
    return q.find(path).isEmpty();
}

template <typename T>
StatusOr<std::vector<T>> JsonReader::getVector(const std::string& path) const {
    auto arr = getArray(path);
    if (!arr.ok()) return StatusOr<std::vector<T>>::Fail(arr.status());
    std::vector<T> out;
    out.reserve(arr.value()->size());
    for (std::size_t i = 0; i < arr.value()->size(); ++i) {
        try {
            out.push_back(arr.value()->get(i).convert<T>());
        } catch (...) {
            return StatusOr<std::vector<T>>::Fail(
                ErrorCodeWrapper(static_cast<int>(JsonError::TYPE_MISMATCH), ErrorCategory::JSON));
        }
    }
    return StatusOr<std::vector<T>>::Ok(out);
}

template <typename T>
StatusOr<std::map<std::string, T>> JsonReader::getMap(const std::string& path) const {
    auto keysRes = getKeys(path);
    if (!keysRes.ok()) return StatusOr<std::map<std::string, T>>::Fail(keysRes.status());
    std::map<std::string, T> out;
    for (const auto& k : keysRes.value()) {
        std::string sub = path.empty() ? k : (path + "." + k);
        auto v = getValue<T>(sub);
        if (!v.ok()) return StatusOr<std::map<std::string, T>>::Fail(v.status());
        out[k] = v.value();
    }
    return StatusOr<std::map<std::string, T>>::Ok(out);
}

template <typename T>
StatusOr<std::unordered_map<std::string, T>> JsonReader::getUnorderedMap(const std::string& path) const {
    auto keysRes = getKeys(path);
    if (!keysRes.ok()) return StatusOr<std::unordered_map<std::string, T>>::Fail(keysRes.status());
    std::unordered_map<std::string, T> out;
    for (const auto& k : keysRes.value()) {
        std::string sub = path.empty() ? k : (path + "." + k);
        auto v = getValue<T>(sub);
        if (!v.ok()) return StatusOr<std::unordered_map<std::string, T>>::Fail(v.status());
        out[k] = v.value();
    }
    return StatusOr<std::unordered_map<std::string, T>>::Ok(out);
}

template <typename T>
StatusOr<Poco::Nullable<T>> JsonReader::getValueOptional(const std::string& path) const {
    Poco::JSON::Query q(root_);
    Poco::Dynamic::Var v = q.find(path);
    if (v.isEmpty())
        return StatusOr<Poco::Nullable<T>>::Ok(Poco::Nullable<T>());
    try {
        return StatusOr<Poco::Nullable<T>>::Ok(Poco::Nullable<T>(v.convert<T>()));
    } catch (...) {
        return StatusOr<Poco::Nullable<T>>::Fail(
            ErrorCodeWrapper(static_cast<int>(JsonError::TYPE_MISMATCH), ErrorCategory::JSON));
    }
}

} // namespace json
} // namespace io
} // namespace common
} // namespace AIstudy


#endif // COMMON_IO_JSON_JSON_READER_H
