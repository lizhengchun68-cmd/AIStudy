#ifndef COMMON_IO_JSON_JSON_STREAM_PARSER_H
#define COMMON_IO_JSON_JSON_STREAM_PARSER_H

#include "common/status/exception/error_code_wrapper.h"
#include "common/status/status_or.h"
#include "common/io/json/json_io_options.h"
#include <cstdint>
#include <istream>
#include <string>
#include <vector>

namespace AIstudy {
namespace common {
namespace io {
namespace json {


/**
 * @brief SAX-style listener for streaming parse events.
 * Implement this interface and pass to parseStream / parseStreamFromFile.
 */
struct IJsonStreamListener {
    virtual ~IJsonStreamListener() = default;

    virtual void startObject() = 0;
    virtual void endObject() = 0;
    virtual void startArray() = 0;
    virtual void endArray() = 0;
    virtual void key(const std::string& k) = 0;

    virtual void value(int v) = 0;
    virtual void value(unsigned v) = 0;
    virtual void value(std::int64_t v) = 0;
    virtual void value(std::uint64_t v) = 0;
    virtual void value(double d) = 0;
    virtual void value(const std::string& s) = 0;
    virtual void value(bool b) = 0;
    virtual void null() = 0;

    virtual void reset() = 0;
};

/**
 * @brief Streaming JSON parser. Parse from stream/file via SAX callbacks.
 * Uses buffered stream for files; incrementally reads from istream when
 * not using comments/null-byte filters (Poco default).
 */
class JsonStreamParser {
public:
    JsonStreamParser() = default;

    /** @brief Parse from stream. Reads incrementally (no full in-memory copy) when using defaults. */
    StatusOr<bool> parseStream(std::istream& in, IJsonStreamListener& listener);

    /** @brief Parse from file. Uses buffer size from setBufferSize. */
    StatusOr<bool> parseStreamFromFile(const std::string& path, IJsonStreamListener& listener);

    const std::string& getLastParseError() const { return lastParseError_; }
    void setBufferSize(std::size_t n) { bufferSize_ = n; }
    std::size_t getBufferSize() const { return bufferSize_; }

private:
    std::string lastParseError_;
    std::size_t bufferSize_ = kJsonDefaultBufferSize;
    std::vector<char> readBuffer_;
};

} // namespace json
} // namespace io
} // namespace common
} // namespace AIstudy


#endif // COMMON_IO_JSON_JSON_STREAM_PARSER_H
