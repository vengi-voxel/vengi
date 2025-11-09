/**
 * @file
 */

#include "Base64.h"
#include "core/ArrayLength.h"
#include "core/Common.h"
#include "io/MemoryReadStream.h"
#include "io/Stream.h"
#include <ctype.h>

namespace io {
namespace Base64 {
namespace _priv {
/**
 * This array is a LUT specified in RFC 4648
 */
static const char LUT[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
						   'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
						   'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
						   'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'};

static CORE_FORCE_INLINE void encode0(const uint8_t src[3], uint8_t dst[4], core::String &out, int bytes) {
	dst[0] = (src[0] & 0xfc) >> 2;
	dst[1] = ((src[0] & 0x03) << 4) + ((src[1] & 0xf0) >> 4);
	dst[2] = ((src[1] & 0x0f) << 2) + ((src[2] & 0xc0) >> 6);
	dst[3] = src[2] & 0x3f;

	for (int i = 0; i < bytes; ++i) {
		out += _priv::LUT[dst[i]];
	}
}

static CORE_FORCE_INLINE bool decode0(uint8_t src[4], io::WriteStream &out, int bytes) {
	uint8_t dest[3];

	dest[0] = (src[0] << 2) + ((src[1] & 0x30) >> 4);
	dest[1] = ((src[1] & 0xf) << 4) + ((src[2] & 0x3c) >> 2);
	dest[2] = ((src[2] & 0x3) << 6) + src[3];

	out.reserve(bytes);
	for (int i = 0; i < bytes; ++i) {
		if (!out.writeUInt8(dest[i])) {
			return false;
		}
	}
	return true;
}

static CORE_FORCE_INLINE bool validbyte(const uint8_t c) {
	return c == '+' || c == '/' || isalnum(c);
}

} // namespace _priv

core::String encode(io::ReadStream &stream) {
	core::String str;
	uint8_t source[3];
	uint8_t dest[4];

	int bytes = 0;
	while (!stream.eos()) {
		uint8_t val = 0;
		if (stream.readUInt8(val) != 0) {
			return core::String::Empty;
		}
		source[bytes++] = val;
		if (bytes == 3) {
			_priv::encode0(source, dest, str, lengthof(dest));
			bytes = 0;
		}
	}

	// handle remaining bytes
	if (bytes) {
		for (int i = bytes; i < 3; ++i) {
			source[i] = '\0';
		}
		_priv::encode0(source, dest, str, bytes + 1);
		while (bytes++ < 3) {
			str += '=';
		}
	}

	return str;
}

bool decode(io::WriteStream &stream, io::ReadStream &input) {
	int bytes = 0;
	uint8_t src[4];
	int REV_LUT[256];
	for (int i = 0; i < lengthof(_priv::LUT); ++i) {
		REV_LUT[(uint8_t)_priv::LUT[i]] = i;
	}

	while (!input.eos()) {
		uint8_t val = 0;
		if (input.readUInt8(val) != 0) {
			return false;
		}
		if (val == '=') {
			break;
		}
		if (!_priv::validbyte(val)) {
			break;
		}
		src[bytes++] = REV_LUT[val];
		if (bytes == 4) {
			if (!_priv::decode0(src, stream, 3)) {
				return false;
			}
			bytes = 0;
		}
	}

	// handle remaining bytes
	if (bytes) {
		for (int i = bytes; i < 4; i++) {
			src[i] = REV_LUT[0];
		}

		if (!_priv::decode0(src, stream, bytes - 1)) {
			return false;
		}
	}

	return true;
}

bool decode(io::WriteStream &stream, const core::String &input) {
	io::MemoryReadStream inputStream(input.c_str(), input.size());
	return decode(stream, inputStream);
}

} // namespace Base64
} // namespace util
