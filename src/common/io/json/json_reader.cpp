#include "common/io/json/json_reader.h"
#include "common/io/json/json_exception.h"
#include "common/io/json/json_io_options.h"
#include "common/status/exception/error_category.h"
#include "common/status/status_or.h"
#include <Poco/JSON/Parser.h>
#include <Poco/JSON/JSONException.h>
#include <fstream>
#include <functional>
#include <sstream>

namespace AIstudy {
namespace common {
namespace io {
namespace json {


namespace {

ErrorCodeWrapper makeJsonErr(JsonError e) {
    return ErrorCodeWrapper(static_cast<int>(e), ErrorCategory::JSON);
}

} // namespace

bool JsonReader::parseImpl(std::function<Poco::Dynamic::Var()> fn, ErrorCodeWrapper* err, bool isFile, const std::string& pathOrMsg) {
    lastParseError_.clear();
    try {
        root_ = fn();
        return true;
    } catch (const Poco::JSON::JSONException& e) {
        lastParseError_ = e.message();
        auto ec = makeJsonErr(JsonError::PARSE_ERROR);
        if (err) { *err = ec; return false; }
        throw JsonParseException(std::string("parse error: ") + e.message(), ec);
    } catch (const Poco::Exception& e) {
        lastParseError_ = e.message();
        auto ec = isFile ? makeJsonErr(JsonError::FILE_READ_ERROR) : makeJsonErr(JsonError::PARSE_ERROR);
        if (err) { *err = ec; return false; }
        if (isFile)
            throw JsonFileException(std::string("file error: ") + e.message(), ec);
        throw JsonParseException(std::string("parse error: ") + e.message(), ec);
    } catch (const std::exception& e) {
        lastParseError_ = e.what();
        auto ec = makeJsonErr(JsonError::PARSE_ERROR);
        if (err) { *err = ec; return false; }
        throw JsonParseException(std::string("parse error: ") + e.what(), ec);
    }
}

StatusOr<bool> JsonReader::readFile(const std::string& filepath) {
    std::ifstream f;
    if (bufferSize_ > 0) {
        if (readBuffer_.size() != bufferSize_)
            readBuffer_.resize(bufferSize_);
        f.rdbuf()->pubsetbuf(readBuffer_.data(), static_cast<std::streamsize>(readBuffer_.size()));
    }
    f.open(filepath);
    if (!f.good())
        return StatusOr<bool>::Fail(makeJsonErr(JsonError::FILE_READ_ERROR));
    ErrorCodeWrapper err;
    if (!parseImpl([&]() { Poco::JSON::Parser p; return p.parse(f); }, &err, true, filepath))
        return StatusOr<bool>::Fail(err);
    return StatusOr<bool>::Ok(true);
}

StatusOr<bool> JsonReader::readString(const std::string& json) {
    ErrorCodeWrapper err;
    if (!parseImpl([&]() { Poco::JSON::Parser p; return p.parse(json); }, &err, false, json))
        return StatusOr<bool>::Fail(err);
    return StatusOr<bool>::Ok(true);
}

StatusOr<bool> JsonReader::readStream(std::istream& in) {
    ErrorCodeWrapper err;
    if (!parseImpl([&]() { Poco::JSON::Parser p; return p.parse(in); }, &err, false, ""))
        return StatusOr<bool>::Fail(err);
    return StatusOr<bool>::Ok(true);
}

StatusOr<Poco::JSON::Object::Ptr> JsonReader::getObject(const std::string& path) const {
    Poco::JSON::Query q(root_);
    Poco::Dynamic::Var v = q.find(path);
    if (v.isEmpty())
        return StatusOr<Poco::JSON::Object::Ptr>::Fail(
            ErrorCodeWrapper(static_cast<int>(JsonError::KEY_NOT_FOUND), ErrorCategory::JSON));
    auto ptr = q.findObject(path);
    if (!ptr)
        return StatusOr<Poco::JSON::Object::Ptr>::Fail(
            ErrorCodeWrapper(static_cast<int>(JsonError::TYPE_MISMATCH), ErrorCategory::JSON));
    return StatusOr<Poco::JSON::Object::Ptr>::Ok(ptr);
}

StatusOr<Poco::JSON::Array::Ptr> JsonReader::getArray(const std::string& path) const {
    Poco::JSON::Query q(root_);
    Poco::Dynamic::Var v = q.find(path);
    if (v.isEmpty())
        return StatusOr<Poco::JSON::Array::Ptr>::Fail(
            ErrorCodeWrapper(static_cast<int>(JsonError::KEY_NOT_FOUND), ErrorCategory::JSON));
    auto ptr = q.findArray(path);
    if (!ptr)
        return StatusOr<Poco::JSON::Array::Ptr>::Fail(
            ErrorCodeWrapper(static_cast<int>(JsonError::TYPE_MISMATCH), ErrorCategory::JSON));
    return StatusOr<Poco::JSON::Array::Ptr>::Ok(ptr);
}

StatusOr<bool> JsonReader::checkRequiredKeys(std::initializer_list<std::string> paths) const {
    std::vector<std::string> missing;
    for (const auto& p : paths) {
        if (!hasKey(p)) missing.push_back(p);
    }
    if (missing.empty()) return StatusOr<bool>::Ok(true);
    return StatusOr<bool>::Fail(makeJsonErr(JsonError::KEY_NOT_FOUND));
}

bool JsonReader::hasKey(const std::string& path) const {
    Poco::JSON::Query q(root_);
    return !q.find(path).isEmpty();
}

StatusOr<std::vector<std::string>> JsonReader::getKeys(const std::string& path) const {
    auto obj = getObject(path);
    if (!obj.ok()) return StatusOr<std::vector<std::string>>::Fail(obj.status());
    std::vector<std::string> out;
    auto names = obj.value()->getNames();
    for (const auto& n : names) out.push_back(n);
    return StatusOr<std::vector<std::string>>::Ok(out);
}

bool JsonReader::isLoaded() const {
    return !root_.isEmpty();
}

} // namespace json
} // namespace io
} // namespace common
} // namespace AIstudy

