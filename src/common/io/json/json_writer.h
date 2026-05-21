#ifndef COMMON_IO_JSON_JSON_WRITER_H
#define COMMON_IO_JSON_JSON_WRITER_H

#include <cstddef>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <ostream>
#include "common/status/exception/error_code_wrapper.h"
#include "common/status/exception/error_codes.h"
#include "common/status/status_or.h"
#include "common/io/json/json_exception.h"
#include "common/io/json/json_io_options.h"
#include <Poco/Dynamic/Var.h>
#include <Poco/JSON/Object.h>
#include <Poco/JSON/Array.h>

namespace AIstudy {
namespace common {
namespace io {
namespace json {


/**
 * @brief JSON writer (straightforward impl, no factory/PIMPL)
 */
class JsonWriter {
public:
    JsonWriter();

    StatusOr<bool> writeFile(const std::string& filepath);
    StatusOr<bool> writeStream(std::ostream& out);

    std::string writeString();
    std::string writeString(bool prettyPrint);

    void setValue(const std::string& key, const std::string& value);
    void setValue(const std::string& key, int value);
    void setValue(const std::string& key, double value);
    void setValue(const std::string& key, bool value);

    void setObject(const std::string& key, const Poco::JSON::Object::Ptr& obj);
    void setArray(const std::string& key, const Poco::JSON::Array::Ptr& arr);

    template <typename T>
    void setVector(const std::string& key, const std::vector<T>& vec);
    template <typename T>
    void setMap(const std::string& key, const std::map<std::string, T>& m);
    template <typename T>
    void setUnorderedMap(const std::string& key, const std::unordered_map<std::string, T>& m);

    Poco::JSON::Object::Ptr createNestedObject(const std::string& key);
    Poco::JSON::Array::Ptr createNestedArray(const std::string& key);

    void setPrettyPrint(bool enable);
    void setIndent(int spaces);

    /** @brief Buffer size for writeFile (default 8K). §4 大文件缓冲 */
    void setBufferSize(std::size_t n) { bufferSize_ = n; }
    std::size_t getBufferSize() const { return bufferSize_; }

    Poco::JSON::Object::Ptr root() const { return root_; }

private:
    Poco::JSON::Object::Ptr root_;
    unsigned int indent_ = 0;
    int step_ = 2;
    std::size_t bufferSize_ = kJsonDefaultBufferSize;
    std::vector<char> writeBuffer_;
};

template <typename T>
void JsonWriter::setVector(const std::string& key, const std::vector<T>& vec) {
    auto arr = createNestedArray(key);
    for (const auto& e : vec) arr->add(Poco::Dynamic::Var(e));
}

template <typename T>
void JsonWriter::setMap(const std::string& key, const std::map<std::string, T>& m) {
    auto obj = createNestedObject(key);
    for (const auto& kv : m) obj->set(kv.first, Poco::Dynamic::Var(kv.second));
}

template <typename T>
void JsonWriter::setUnorderedMap(const std::string& key, const std::unordered_map<std::string, T>& m) {
    auto obj = createNestedObject(key);
    for (const auto& kv : m) obj->set(kv.first, Poco::Dynamic::Var(kv.second));
}

} // namespace json
} // namespace io
} // namespace common
} // namespace AIstudy


#endif // COMMON_IO_JSON_JSON_WRITER_H
