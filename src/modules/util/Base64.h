/**
 * @file
 */

#include "core/String.h"
#include "io/Stream.h"
#include <stdint.h>

namespace util {
namespace Base64 {

core::String encode(const uint8_t *buf, size_t len);
core::String encode(io::ReadStream &stream);
bool decode(io::WriteStream &stream, const core::String &input);

} // namespace Base64
} // namespace util
