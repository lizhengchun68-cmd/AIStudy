#ifndef COMMON_IO_JSON_JSON_ADVANCED_H
#define COMMON_IO_JSON_JSON_ADVANCED_H

#include "common/status/status_or.h"
#include "common/io/json/json_reader.h"
#include "common/io/json/json_writer.h"
#include <Poco/JSON/Object.h>
#include <future>
#include <string>
#include <utility>
#include <vector>

namespace AIstudy {
namespace common {
namespace io {
namespace json {


/** @brief Batch read: one StatusOr<Object::Ptr> per path. Root must be JSON object. */
std::vector<StatusOr<Poco::JSON::Object::Ptr>> readJsonFiles(
    const std::vector<std::string>& paths);

/** @brief Batch write: (path, object). Fails on first error. */
StatusOr<bool> writeJsonFiles(
    const std::vector<std::pair<std::string, Poco::JSON::Object::Ptr>>& pathAndObjects);

/** @brief Read fragment: parse file, return getValue<T>(path). */
template <typename T>
StatusOr<T> readJsonFileFragment(const std::string& filepath, const std::string& path) {
    JsonReader r;
    auto ok = r.readFile(filepath);
    if (!ok.ok()) return StatusOr<T>::Fail(ok.status());
    return r.getValue<T>(path);
}

/** @brief Parse JSON string after stripping line and block comments (JSON_IO SS5). */
StatusOr<bool> readJsonStringAllowComments(JsonReader& r, const std::string& json);

/** @brief Read file, strip comments, parse. */
StatusOr<bool> readJsonFileAllowComments(JsonReader& r, const std::string& filepath);

/** @brief Async read file -> Object::Ptr. */
std::future<StatusOr<Poco::JSON::Object::Ptr>> readJsonFileAsync(const std::string& path);

/** @brief Async write Object to file. */
std::future<StatusOr<bool>> writeJsonFileAsync(const std::string& path,
                                               const Poco::JSON::Object::Ptr& obj);

} // namespace json
} // namespace io
} // namespace common
} // namespace AIstudy


#endif // COMMON_IO_JSON_JSON_ADVANCED_H
