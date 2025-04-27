/**
 * @file
 */

#include "io/TokenStream.h"

namespace io {

core::String TokenStream::next() {
	if (_stream.eos()) {
		return core::String::Empty;
	}
	core::String token;
	while (!_stream.eos()) {
		uint8_t c;
		if (_stream.readUInt8(c) == -1) {
			return core::String::Empty;
		}
		if (c == '\t' || c == ' ' || c == '\n' || c == '\r') {
			if (!token.empty()) {
				break;
			}
			continue;
		}
		token += c;
	}
	return token;
}

} // namespace io
