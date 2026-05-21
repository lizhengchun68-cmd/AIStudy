#include "common/io/json/json_advanced.h"
#include "common/io/json/json_reader.h"
#include "common/io/json/json_writer.h"
#include "common/io/json/json_io_options.h"
#include "common/status/exception/error_code_wrapper.h"
#include "common/status/exception/error_category.h"
#include "common/status/status_or.h"
#include "common/status/exception/error_codes.h"
#include <Poco/JSON/Parser.h>
#include <Poco/JSON/Stringifier.h>
#include <Poco/File.h>
#include <Poco/Path.h>
#include <fstream>
#include <future>
#include <mutex>
#include <sstream>

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

/** Strip line and block comments, respecting quoted strings and escapes. */
std::string stripJsonComments(const std::string& raw) {
    std::string out;
    out.reserve(raw.size());
    enum { Normal, InDouble, InEscape } state = Normal;
    std::size_t i = 0;
    while (i < raw.size()) {
        char c = raw[i];
        if (state == Normal) {
            if (c == '"') {
                out += c;
                state = InDouble;
                ++i;
                continue;
            }
            if (c == '/' && i + 1 < raw.size()) {
                if (raw[i + 1] == '/') {
                    while (i < raw.size() && raw[i] != '\n') ++i;
                    if (i < raw.size()) { out += '\n'; ++i; }
                    continue;
                }
                if (raw[i + 1] == '*') {
                    i += 2;
                    while (i + 1 < raw.size() && !(raw[i] == '*' && raw[i + 1] == '/')) ++i;
                    if (i + 1 < raw.size()) i += 2;
                    continue;
                }
            }
            out += c;
            ++i;
            continue;
        }
        if (state == InDouble) {
            if (c == '\\' && i + 1 < raw.size()) {
                out += c;
                out += raw[i + 1];
                i += 2;
                continue;
            }
            if (c == '"') {
                out += c;
                state = Normal;
                ++i;
                continue;
            }
            out += c;
            ++i;
            continue;
        }
        out += c;
        ++i;
    }
    return out;
}

bool atomicWriteObject(const std::string& filepath, const Poco::JSON::Object::Ptr& obj,
                       std::vector<char>& buf, std::size_t bufSize) {
    Poco::Path p(filepath);
    std::string parent = p.parent().toString();
    if (!parent.empty() && parent != ".") {
        Poco::File(parent).createDirectories();
    }
    std::string tmp = filepath + ".tmp";
    if (bufSize > 0 && buf.size() != bufSize)
        buf.resize(bufSize);
    std::ofstream f;
    if (bufSize > 0 && buf.size() > 0)
        f.rdbuf()->pubsetbuf(buf.data(), static_cast<std::streamsize>(buf.size()));
    f.open(tmp);
    if (!f.good()) return false;
    Poco::JSON::Stringifier::condense(Poco::Dynamic::Var(obj), f);
    f.flush();
    if (!f.good()) return false;
    f.close();
    Poco::File tmpFile(tmp);
    if (!tmpFile.exists()) return false;
    tmpFile.renameTo(filepath);
    return true;
}

} // namespace

std::vector<StatusOr<Poco::JSON::Object::Ptr>> readJsonFiles(
    const std::vector<std::string>& paths) {
    std::vector<StatusOr<Poco::JSON::Object::Ptr>> out;
    out.reserve(paths.size());
    for (const auto& path : paths) {
        std::ifstream f(path);
        if (!f.good()) {
            out.push_back(StatusOr<Poco::JSON::Object::Ptr>::Fail(makeJsonErr(JsonError::FILE_READ_ERROR)));
            continue;
        }
        try {
            Poco::JSON::Parser parser;
            auto var = parser.parse(f);
            if (var.isEmpty()) {
                out.push_back(StatusOr<Poco::JSON::Object::Ptr>::Fail(makeJsonErr(JsonError::PARSE_ERROR)));
                continue;
            }
            auto ptr = var.extract<Poco::JSON::Object::Ptr>();
            out.push_back(StatusOr<Poco::JSON::Object::Ptr>::Ok(ptr));
        } catch (const Poco::Exception&) {
            out.push_back(StatusOr<Poco::JSON::Object::Ptr>::Fail(makeJsonErr(JsonError::PARSE_ERROR)));
        } catch (const std::exception&) {
            out.push_back(StatusOr<Poco::JSON::Object::Ptr>::Fail(makeJsonErr(JsonError::PARSE_ERROR)));
        }
    }
    return out;
}

StatusOr<bool> writeJsonFiles(
    const std::vector<std::pair<std::string, Poco::JSON::Object::Ptr>>& pathAndObjects) {
    std::lock_guard<std::mutex> lock(jsonWriteMutex());
    std::vector<char> buf(kJsonDefaultBufferSize);
    for (const auto& p : pathAndObjects) {
        if (!p.second)
            return StatusOr<bool>::Fail(makeJsonErr(JsonError::SERIALIZE_ERROR));
        try {
            if (!atomicWriteObject(p.first, p.second, buf, buf.size()))
                return StatusOr<bool>::Fail(makeJsonErr(JsonError::FILE_WRITE_ERROR));
        } catch (const Poco::JSON::JSONException&) {
            return StatusOr<bool>::Fail(makeJsonErr(JsonError::SERIALIZE_ERROR));
        } catch (const Poco::Exception&) {
            return StatusOr<bool>::Fail(makeJsonErr(JsonError::FILE_WRITE_ERROR));
        }
    }
    return StatusOr<bool>::Ok(true);
}

StatusOr<bool> readJsonStringAllowComments(JsonReader& r, const std::string& json) {
    std::string stripped = stripJsonComments(json);
    return r.readString(stripped);
}

StatusOr<bool> readJsonFileAllowComments(JsonReader& r, const std::string& filepath) {
    std::ifstream f(filepath);
    if (!f.good())
        return StatusOr<bool>::Fail(makeJsonErr(JsonError::FILE_READ_ERROR));
    std::ostringstream oss;
    oss << f.rdbuf();
    return readJsonStringAllowComments(r, oss.str());
}

std::future<StatusOr<Poco::JSON::Object::Ptr>> readJsonFileAsync(const std::string& path) {
    return std::async(std::launch::async, [path]() -> StatusOr<Poco::JSON::Object::Ptr> {
        std::vector<std::string> one = { path };
        auto out = readJsonFiles(one);
        return out.empty() ? StatusOr<Poco::JSON::Object::Ptr>::Fail(makeJsonErr(JsonError::FILE_READ_ERROR))
                           : std::move(out[0]);
    });
}

std::future<StatusOr<bool>> writeJsonFileAsync(const std::string& path,
                                               const Poco::JSON::Object::Ptr& obj) {
    Poco::JSON::Object::Ptr copy = obj;  // capture by value for async
    return std::async(std::launch::async, [path, copy]() -> StatusOr<bool> {
        std::vector<std::pair<std::string, Poco::JSON::Object::Ptr>> one = { { path, copy } };
        return writeJsonFiles(one);
    });
}

} // namespace json
} // namespace io
} // namespace common
} // namespace AIstudy

