/**
 * @file
 */

#include "core/String.h"
#include "io/Stream.h"

namespace io {
namespace Base64 {

core::String encode(io::ReadStream &stream);
bool decode(io::WriteStream &stream, const core::String &input);
bool decode(io::WriteStream &stream, io::ReadStream &input);

} // namespace Base64
} // namespace util
