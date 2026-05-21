#include "common/io/json/json_writer.h"
#include "common/status/exception/error_category.h"
#include "common/status/status_or.h"
#include "common/io/json/json_io_options.h"
#include <Poco/JSON/Stringifier.h>
#include <Poco/JSON/JSONException.h>
#include <Poco/File.h>
#include <Poco/Path.h>
#include <fstream>
#include <sstream>
#include <mutex>

namespace AIstudy {
namespace common {
namespace io {
namespace json {


namespace {

ErrorCodeWrapper makeJsonErr(JsonError e) {
    return ErrorCodeWrapper(static_cast<int>(e), ErrorCategory::JSON);
}

std::mutex& jsonWriteMutex() {
    static std::mutex m;
    return m;
}

} // namespace

JsonWriter::JsonWriter() : root_(new Poco::JSON::Object()) {}

void JsonWriter::setPrettyPrint(bool enable) {
    if (enable) { indent_ = 0; step_ = 2; }
    else { indent_ = 0; step_ = -1; }
}

void JsonWriter::setIndent(int spaces) {
    step_ = spaces <= 0 ? -1 : spaces;
}

void JsonWriter::setValue(const std::string& key, const std::string& value) {
    root_->set(key, Poco::Dynamic::Var(value));
}

void JsonWriter::setValue(const std::string& key, int value) {
    root_->set(key, Poco::Dynamic::Var(value));
}

void JsonWriter::setValue(const std::string& key, double value) {
    root_->set(key, Poco::Dynamic::Var(value));
}

void JsonWriter::setValue(const std::string& key, bool value) {
    root_->set(key, Poco::Dynamic::Var(value));
}

void JsonWriter::setObject(const std::string& key, const Poco::JSON::Object::Ptr& obj) {
    root_->set(key, Poco::Dynamic::Var(obj));
}

void JsonWriter::setArray(const std::string& key, const Poco::JSON::Array::Ptr& arr) {
    root_->set(key, Poco::Dynamic::Var(arr));
}

Poco::JSON::Object::Ptr JsonWriter::createNestedObject(const std::string& key) {
    auto obj = new Poco::JSON::Object();
    Poco::JSON::Object::Ptr p(obj);
    root_->set(key, Poco::Dynamic::Var(p));
    return p;
}

Poco::JSON::Array::Ptr JsonWriter::createNestedArray(const std::string& key) {
    auto arr = new Poco::JSON::Array();
    Poco::JSON::Array::Ptr p(arr);
    root_->set(key, Poco::Dynamic::Var(p));
    return p;
}

StatusOr<bool> JsonWriter::writeFile(const std::string& filepath) {
    std::lock_guard<std::mutex> lock(jsonWriteMutex());
    try {
        Poco::Path p(filepath);
        std::string parent = p.parent().toString();
        if (!parent.empty() && parent != ".") {
            Poco::File(parent).createDirectories();
        }
        std::string tmp = filepath + ".tmp";
        {
            std::ofstream f;
            if (bufferSize_ > 0) {
                if (writeBuffer_.size() != bufferSize_)
                    writeBuffer_.resize(bufferSize_);
                f.rdbuf()->pubsetbuf(writeBuffer_.data(), static_cast<std::streamsize>(writeBuffer_.size()));
            }
            f.open(tmp);
            if (!f.good())
                return StatusOr<bool>::Fail(makeJsonErr(JsonError::FILE_WRITE_ERROR));
            Poco::JSON::Stringifier::stringify(Poco::Dynamic::Var(root_), f, indent_, step_);
            f.flush();
            if (!f.good())
                return StatusOr<bool>::Fail(makeJsonErr(JsonError::FILE_WRITE_ERROR));
        }
        Poco::File tmpFile(tmp);
        if (!tmpFile.exists())
            return StatusOr<bool>::Fail(makeJsonErr(JsonError::FILE_WRITE_ERROR));
        tmpFile.renameTo(filepath);
        return StatusOr<bool>::Ok(true);
    } catch (const Poco::JSON::JSONException&) {
        return StatusOr<bool>::Fail(makeJsonErr(JsonError::SERIALIZE_ERROR));
    } catch (const Poco::Exception&) {
        return StatusOr<bool>::Fail(makeJsonErr(JsonError::FILE_WRITE_ERROR));
    } catch (const std::exception&) {
        return StatusOr<bool>::Fail(makeJsonErr(JsonError::SERIALIZE_ERROR));
    }
}

std::string JsonWriter::writeString() {
    return writeString(step_ > 0);
}

std::string JsonWriter::writeString(bool prettyPrint) {
    std::ostringstream oss;
    if (prettyPrint)
        Poco::JSON::Stringifier::stringify(Poco::Dynamic::Var(root_), oss, 0, 2);
    else
        Poco::JSON::Stringifier::condense(Poco::Dynamic::Var(root_), oss);
    return oss.str();
}

StatusOr<bool> JsonWriter::writeStream(std::ostream& out) {
    try {
        Poco::JSON::Stringifier::stringify(Poco::Dynamic::Var(root_), out, indent_, step_);
        return StatusOr<bool>::Ok(true);
    } catch (const Poco::JSON::JSONException&) {
        return StatusOr<bool>::Fail(makeJsonErr(JsonError::SERIALIZE_ERROR));
    } catch (const std::exception&) {
        return StatusOr<bool>::Fail(makeJsonErr(JsonError::SERIALIZE_ERROR));
    }
}

} // namespace json
} // namespace io
} // namespace common
} // namespace AIstudy

