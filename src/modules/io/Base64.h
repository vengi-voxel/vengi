/**
 * @file
 */

#pragma once

#include "core/String.h"
#include "io/Stream.h"

namespace io {
namespace Base64 {

/** Check standard alphabet, four-character groups and trailing padding; empty is valid. */
bool isValid(const core::String &input);

core::String encode(io::ReadStream &stream);
bool decode(io::WriteStream &stream, const core::String &input);
bool decode(io::WriteStream &stream, io::ReadStream &input);
core::String base64urlToBase64(const core::String &input);

} // namespace Base64
} // namespace util
