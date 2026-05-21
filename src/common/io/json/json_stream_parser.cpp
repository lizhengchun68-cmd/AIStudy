#include "common/io/json/json_stream_parser.h"
#include "common/status/exception/error_codes.h"
#include "common/status/exception/error_category.h"
#include "common/status/status_or.h"
#include <Poco/JSON/Parser.h>
#include <Poco/JSON/Handler.h>
#include <Poco/JSON/JSONException.h>
#include <fstream>
#include <sstream>

namespace AIstudy {
namespace common {
namespace io {
namespace json {


namespace {

ErrorCodeWrapper makeJsonErr(JsonError e) {
    return ErrorCodeWrapper(static_cast<int>(e), ErrorCategory::JSON);
}

/** Forwards Poco::JSON::Handler events to IJsonStreamListener. */
class StreamHandler : public Poco::JSON::Handler {
public:
    explicit StreamHandler(IJsonStreamListener& l) : listener_(l) {}

    void reset() override { listener_.reset(); }
    void startObject() override { listener_.startObject(); }
    void endObject() override { listener_.endObject(); }
    void startArray() override { listener_.startArray(); }
    void endArray() override { listener_.endArray(); }
    void key(const std::string& k) override { listener_.key(k); }
    void null() override { listener_.null(); }

    void value(int v) override { listener_.value(v); }
    void value(unsigned v) override { listener_.value(v); }
#if defined(POCO_HAVE_INT64)
    void value(Poco::Int64 v) override { listener_.value(static_cast<std::int64_t>(v)); }
    void value(Poco::UInt64 v) override { listener_.value(static_cast<std::uint64_t>(v)); }
#endif
    void value(const std::string& s) override { listener_.value(s); }
    void value(double d) override { listener_.value(d); }
    void value(bool b) override { listener_.value(b); }

private:
    IJsonStreamListener& listener_;
};

} // namespace

StatusOr<bool> JsonStreamParser::parseStream(std::istream& in, IJsonStreamListener& listener) {
    lastParseError_.clear();
    try {
        listener.reset();
        Poco::JSON::Handler::Ptr handler(new StreamHandler(listener));
        Poco::JSON::Parser parser(handler);
        parser.parse(in);
        return StatusOr<bool>::Ok(true);
    } catch (const Poco::JSON::JSONException& e) {
        lastParseError_ = e.message();
        return StatusOr<bool>::Fail(makeJsonErr(JsonError::PARSE_ERROR));
    } catch (const Poco::Exception& e) {
        lastParseError_ = e.message();
        return StatusOr<bool>::Fail(makeJsonErr(JsonError::PARSE_ERROR));
    } catch (const std::exception& e) {
        lastParseError_ = e.what();
        return StatusOr<bool>::Fail(makeJsonErr(JsonError::PARSE_ERROR));
    }
}

StatusOr<bool> JsonStreamParser::parseStreamFromFile(const std::string& path,
                                                     IJsonStreamListener& listener) {
    lastParseError_.clear();
    std::ifstream f;
    if (bufferSize_ > 0) {
        if (readBuffer_.size() != bufferSize_)
            readBuffer_.resize(bufferSize_);
        f.rdbuf()->pubsetbuf(readBuffer_.data(), static_cast<std::streamsize>(readBuffer_.size()));
    }
    f.open(path);
    if (!f.good())
        return StatusOr<bool>::Fail(makeJsonErr(JsonError::FILE_READ_ERROR));
    return parseStream(f, listener);
}

} // namespace json
} // namespace io
} // namespace common
} // namespace AIstudy

