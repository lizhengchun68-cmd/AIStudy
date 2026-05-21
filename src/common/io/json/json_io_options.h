#ifndef COMMON_IO_JSON_JSON_IO_OPTIONS_H
#define COMMON_IO_JSON_JSON_IO_OPTIONS_H

#include <cstddef>

namespace AIstudy {
namespace common {
namespace io {
namespace json {


/** @brief Default buffer size for file I/O (8KB). */
constexpr std::size_t kJsonDefaultBufferSize = 8192;

/** @brief I/O options: buffer size for large-file read/write. */
struct JsonIOOptions {
    std::size_t bufferSize = kJsonDefaultBufferSize;
};

} // namespace json
} // namespace io
} // namespace common
} // namespace AIstudy


#endif // COMMON_IO_JSON_JSON_IO_OPTIONS_H
