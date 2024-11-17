/**
 * @file
 *
 * Ascii85 is an encoding scheme that uses 7-bit printable ASCII characters, also known as Base85.
 *
 * This implements the ZeroMQ variant that is called Z85
 */

#include "io/Stream.h"

namespace io {
namespace Z85 {

core::String encode(io::ReadStream &stream);
bool decode(io::WriteStream &stream, io::ReadStream &input);
bool decode(io::WriteStream &stream, const core::String &input);

} // namespace Z85
} // namespace io
