/**
 * @file
 */

#include "Z85.h"
#include "core/ArrayLength.h"
#include "core/Common.h"
#include "io/MemoryReadStream.h"
#include "io/Stream.h"

namespace io {
namespace Z85 {
namespace _priv {

// Z85 encoding table
static const uint8_t LUT[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f', 'g',
							  'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x',
							  'y', 'z', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
							  'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '.', '-', ':', '+', '=', '^',
							  '!', '/', '*', '?', '&', '<', '>', '(', ')', '[', ']', '{', '}', '@', '%', '$', '#'};

// Encode a 4-byte chunk into a 5-character Z85 string
static CORE_FORCE_INLINE void encodeChunk(const uint8_t src[4], core::String &out) {
	uint32_t value = (src[0] << 24) | (src[1] << 16) | (src[2] << 8) | src[3];
	char dest[5];
	for (int i = 4; i >= 0; --i) {
		dest[i] = LUT[value % 85];
		value /= 85;
	}
	out.append(dest, 5);
}

} // namespace _priv

core::String encode(io::ReadStream &stream) {
	core::String out;
	uint8_t source[4];
	int bytes = 0;

	while (!stream.eos()) {
		uint8_t val = 0;
		if (stream.readUInt8(val) != 0) {
			return core::String::Empty; // Error reading input
		}
		source[bytes++] = val;

		if (bytes == 4) {
			_priv::encodeChunk(source, out);
			bytes = 0;
		}
	}

	// Handle padding (input size not divisible by 4)
	if (bytes > 0) {
		for (int i = bytes; i < 4; ++i) {
			source[i] = 0; // Pad with zero bytes
		}
		_priv::encodeChunk(source, out);
		out = out.substr(0, out.size() - (4 - bytes)); // Remove padding characters
	}

	return out;
}

bool decode(io::WriteStream &stream, io::ReadStream &input) {
	uint8_t source[5];
	int bytes = 0;

	// Z85 decoding table (reverse LUT)
	int32_t REV_LUT[256];

	// Initialize the reverse LUT
	for (int i = 0; i < lengthof(REV_LUT); ++i) {
		REV_LUT[i] = -1;
	}
	for (int i = 0; i < lengthof(_priv::LUT); ++i) {
		REV_LUT[_priv::LUT[i]] = i;
	}

	// Main loop: Process complete 5-character chunks
	while (!input.eos()) {
		uint8_t val = 0;
		if (input.readUInt8(val) == -1) {
			return false; // Error reading input
		}
		source[bytes++] = val;

		if (bytes == 5) {
			// Decode a 5-character Z85 string into a 4-byte chunk
			uint32_t value = 0;
			for (int i = 0; i < 5; ++i) {
				int32_t idx = REV_LUT[source[i]];
				if (idx == -1) {
					return false; // Invalid character in input
				}
				value = value * 85 + idx;
			}
			uint8_t dest[4] = {(uint8_t)((value >> 24) & 0xFF), (uint8_t)((value >> 16) & 0xFF),
							   (uint8_t)((value >> 8) & 0xFF), (uint8_t)(value & 0xFF)};
			if (stream.write(dest, 4) == -1) {
				return false; // Failed to write chunk
			}
			bytes = 0; // Reset counter after processing chunk
		}
	}

	// Handle remaining bytes (input size not divisible by 5)
	if (bytes > 0) {
		for (int i = bytes; i < 5; ++i) {
			source[i] = 'u'; // 'u' maps to 0 in Z85, padding the incomplete chunk
		}

		// Decode padded chunk
		uint32_t value = 0;
		for (int i = 0; i < 5; ++i) {
			int32_t idx = REV_LUT[source[i]];
			if (idx == -1) {
				return false; // Invalid character in input
			}
			value = value * 85 + idx;
		}
		uint8_t dest[4] = {(uint8_t)((value >> 24) & 0xFF), (uint8_t)((value >> 16) & 0xFF),
						   (uint8_t)((value >> 8) & 0xFF), (uint8_t)(value & 0xFF)};
		if (stream.write(dest, bytes - 1) == -1) { // Write only the original bytes (exclude padding)
			return false;
		}
	}

	return true; // Successfully decoded
}

bool decode(io::WriteStream &stream, const core::String &input) {
	io::MemoryReadStream inputStream(input.c_str(), input.size());
	return decode(stream, inputStream);
}

} // namespace Z85
} // namespace io
