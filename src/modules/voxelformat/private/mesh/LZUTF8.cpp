/**
 * @file
 */

#include "LZUTF8.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "core/Unicode.h"
#include "core/collection/Buffer.h"

namespace voxelformat {
namespace lzutf8 {

bool decodeStorageBinaryString(const uint16_t *chars, size_t charCount, core::Buffer<uint8_t> &out) {
	out.clear();
	if (chars == nullptr || charCount == 0) {
		return true;
	}

	out.reserve(charCount * 2);
	uint32_t remainder = 0;
	int state = 0;

	for (size_t i = 0; i < charCount; ++i) {
		uint32_t value = chars[i];
		// StorageBinaryString uses U+8002 as a stand-in for NUL in BinaryString
		if (value == 0x8002) {
			value = 0;
		}

		if (value >= 32768) {
			if (value == (32768 | 1)) {
				if (!out.empty()) {
					out.pop();
				}
			}
			state = 0;
			continue;
		}

		if (state == 0) {
			remainder = value;
		} else {
			const uint32_t combined = (remainder << state) | (value >> (15 - state));
			out.push_back((uint8_t)(combined >> 8));
			out.push_back((uint8_t)(combined & 255));
			remainder = value & ((1u << (15 - state)) - 1u);
		}

		if (state == 15) {
			state = 0;
		} else {
			++state;
		}
	}
	return true;
}

bool encodeStorageBinaryString(const uint8_t *bytes, size_t byteCount, core::Buffer<uint16_t> &out) {
	out.clear();
	if (bytes == nullptr || byteCount == 0) {
		return true;
	}

	uint32_t remainder = 0;
	int state = 1;
	for (size_t i = 0; i < byteCount; i += 2) {
		uint32_t value;
		if (i == byteCount - 1) {
			value = ((uint32_t)bytes[i] << 8);
		} else {
			value = ((uint32_t)bytes[i] << 8) | bytes[i + 1];
		}

		uint16_t ch = (uint16_t)((remainder << (16 - state)) | (value >> state));
		if (ch == 0) {
			ch = 0x8002;
		}
		out.push_back(ch);
		remainder = value & ((1u << state) - 1u);

		if (state == 15) {
			uint16_t remCh = (uint16_t)remainder;
			if (remCh == 0) {
				remCh = 0x8002;
			}
			out.push_back(remCh);
			remainder = 0;
			state = 1;
		} else {
			++state;
		}

		if (i >= byteCount - 2) {
			uint16_t remCh = (uint16_t)(remainder << (16 - state));
			if (remCh == 0) {
				remCh = 0x8002;
			}
			out.push_back(remCh);
		}
	}
	out.push_back((uint16_t)(32768 | (byteCount % 2)));
	return true;
}

bool decompress(const uint8_t *input, size_t inputSize, core::Buffer<uint8_t> &out) {
	out.clear();
	if (input == nullptr || inputSize == 0) {
		return true;
	}

	out.reserve(inputSize * 2);
	for (size_t readPosition = 0; readPosition < inputSize; ++readPosition) {
		const uint8_t inputValue = input[readPosition];

		if ((inputValue >> 6) != 3) {
			out.push_back(inputValue);
			continue;
		}

		const int sequenceLengthIdentifier = inputValue >> 5; // 6 for 2 bytes, 7 for at least 3
		if (readPosition == inputSize - 1 || (readPosition == inputSize - 2 && sequenceLengthIdentifier == 7)) {
			Log::error("Truncated LZUTF8 pointer sequence");
			return false;
		}

		if ((input[readPosition + 1] >> 7) == 1) {
			out.push_back(inputValue);
			continue;
		}

		const int matchLength = inputValue & 31;
		int matchDistance;
		if (sequenceLengthIdentifier == 6) {
			matchDistance = input[readPosition + 1];
			readPosition += 1;
		} else {
			matchDistance = (input[readPosition + 1] << 8) | input[readPosition + 2];
			readPosition += 2;
		}

		if (matchDistance <= 0 || matchDistance > (int)out.size() || matchLength <= 0) {
			Log::error("Invalid LZUTF8 match distance=%d length=%d outSize=%d", matchDistance, matchLength,
					   (int)out.size());
			return false;
		}

		const size_t matchPosition = out.size() - (size_t)matchDistance;
		for (int offset = 0; offset < matchLength; ++offset) {
			const uint8_t b = out[matchPosition + (size_t)offset];
			out.push_back(b);
		}
	}
	return true;
}

core::String stripJsonComments(const core::String &input) {
	core::String out;
	out.reserve(input.size());
	bool inString = false;
	bool escape = false;
	for (size_t i = 0; i < input.size(); ++i) {
		const char c = input[i];
		if (inString) {
			out.append(&c, 1);
			if (escape) {
				escape = false;
			} else if (c == '\\') {
				escape = true;
			} else if (c == '"') {
				inString = false;
			}
			continue;
		}
		if (c == '"') {
			inString = true;
			out.append(&c, 1);
			continue;
		}
		if (c == '/' && i + 1 < input.size()) {
			if (input[i + 1] == '/') {
				i += 2;
				while (i < input.size() && input[i] != '\n') {
					++i;
				}
				if (i < input.size()) {
					const char nl = '\n';
					out.append(&nl, 1);
				}
				continue;
			}
			if (input[i + 1] == '*') {
				i += 2;
				while (i + 1 < input.size() && !(input[i] == '*' && input[i + 1] == '/')) {
					++i;
				}
				if (i + 1 < input.size()) {
					i += 1; // skip closing /
				}
				continue;
			}
		}
		out.append(&c, 1);
	}
	return out;
}

static bool utf8ToCodeUnits(const char *utf8, size_t size, core::Buffer<uint16_t> &out) {
	out.clear();
	const char *p = utf8;
	const char *end = utf8 + size;
	while (p < end && *p != '\0') {
		const char *prev = p;
		const int cp = core::unicode::next(&p);
		if (cp < 0) {
			break;
		}
		if (p == prev) {
			++p;
			continue;
		}
		if (cp <= 0xFFFF) {
			out.push_back((uint16_t)cp);
		} else {
			// unexpected for StorageBinaryString payload
			const int c = cp - 0x10000;
			out.push_back((uint16_t)(0xD800 + (c >> 10)));
			out.push_back((uint16_t)(0xDC00 + (c & 0x3FF)));
		}
	}
	return true;
}

core::String decodeBBModelText(const core::String &raw) {
	core::String text = raw;
	if (core::string::startsWith(text, "<lz>")) {
		const char *payload = text.c_str() + 4;
		const size_t payloadBytes = text.size() >= 4 ? text.size() - 4 : 0;
		core::Buffer<uint16_t> codeUnits;
		if (!utf8ToCodeUnits(payload, payloadBytes, codeUnits)) {
			Log::error("Failed to decode UTF-8 for lzutf8 payload");
			return core::String::Empty;
		}
		core::Buffer<uint8_t> compressed;
		if (!decodeStorageBinaryString(codeUnits.data(), codeUnits.size(), compressed)) {
			Log::error("Failed to decode StorageBinaryString");
			return core::String::Empty;
		}
		core::Buffer<uint8_t> decompressed;
		if (!decompress(compressed.data(), compressed.size(), decompressed)) {
			Log::error("Failed to decompress LZUTF8 bbmodel payload");
			return core::String::Empty;
		}
		text = core::String((const char *)decompressed.data(), decompressed.size());
	}

	// Strip UTF-8 BOM (U+FEFF)
	if (text.size() >= 3 && (uint8_t)text[0] == 0xEF && (uint8_t)text[1] == 0xBB && (uint8_t)text[2] == 0xBF) {
		text = text.substr(3);
	}

	return text;
}

} // namespace lzutf8
} // namespace voxelformat
