/**
 * @file
 */

#include "Base64ReadStream.h"
#include "io/Stream.h"

namespace io {

CORE_FORCE_INLINE static constexpr int reverseLookup(uint8_t c) {
	return (c >= 'A' && c <= 'Z')	? (c - 'A')
		   : (c >= 'a' && c <= 'z') ? (c - 'a' + 26)
		   : (c >= '0' && c <= '9') ? (c - '0' + 52)
		   : (c == '+')				? 62
		   : (c == '/')				? 63
									: -1;
}

CORE_FORCE_INLINE static int decodeBlock(io::ReadStream &stream, uint8_t *dest) {
	uint8_t encoded[4];
	if (stream.read(encoded, 4) != 4) {
		return -1;
	}

	// Determine decoded size from padding
	int decodedSize = 3;
	if (encoded[3] == '=') {
		decodedSize = (encoded[2] == '=') ? 1 : 2;
	}

	// Reverse lookup all 4 characters (padding treated as 0)
	const int v0 = reverseLookup(encoded[0]);
	const int v1 = reverseLookup(encoded[1]);
	const int v2 = (encoded[2] == '=') ? 0 : reverseLookup(encoded[2]);
	const int v3 = (encoded[3] == '=') ? 0 : reverseLookup(encoded[3]);

	if (core_unlikely((v0 | v1 | v2 | v3) < 0)) {
		return -1;
	}

	// Decode the block
	dest[0] = (uint8_t)((v0 << 2) | (v1 >> 4));
	dest[1] = (uint8_t)((v1 << 4) | (v2 >> 2));
	dest[2] = (uint8_t)((v2 << 6) | v3);

	return decodedSize;
}

Base64ReadStream::Base64ReadStream(io::ReadStream &stream) : _stream(stream) {
}

int Base64ReadStream::read(void *buf, size_t size) {
	uint8_t *bytesPtr = (uint8_t *)buf;
	size_t bytesWritten = 0;

	// Flush any remaining cached bytes first
	while (_readBufSize > 0 && bytesWritten < size) {
		bytesPtr[bytesWritten++] = _readBuf[_readBufPos++];
		--_readBufSize;
	}
	if (bytesWritten >= size) {
		return bytesWritten;
	}

	// Fast path: decode directly into output buffer in batches
	while (bytesWritten + 3 <= size) {
		if (_stream.eos()) {
			return bytesWritten;
		}

		// Try to process multiple blocks at once (up to 256 blocks = 1024 encoded bytes)
		const size_t remainingBytes = size - bytesWritten;
		const size_t blocksToProcess = (remainingBytes / 3 < 256) ? (remainingBytes / 3) : 256;
		const size_t encodedBytesToRead = blocksToProcess * 4;

		uint8_t encodedBatch[1024]; // 256 blocks * 4 bytes
		const int bytesRead = _stream.read(encodedBatch, encodedBytesToRead);

		if (bytesRead < 4) {
			if (bytesRead > 0) {
				return -1; // Incomplete block
			}
			return bytesWritten;
		}

		// Process all complete 4-byte blocks
		const int completeBlocks = bytesRead >> 2; // Divide by 4
		const uint8_t *encoded = encodedBatch;
		uint8_t *dest = bytesPtr + bytesWritten;

		// Process blocks one at a time
		for (int blockIdx = 0; blockIdx < completeBlocks; ++blockIdx) {
			const uint8_t e0 = encoded[0];
			const uint8_t e1 = encoded[1];
			const uint8_t e2 = encoded[2];
			const uint8_t e3 = encoded[3];

			// Check for padding
			const bool hasPadding = (e3 == '=');
			if (hasPadding) {
				const int decodedSize = (e2 == '=') ? 1 : 2;

				// Reverse lookup
				const int v0 = reverseLookup(e0);
				const int v1 = reverseLookup(e1);
				const int v2 = (e2 == '=') ? 0 : reverseLookup(e2);

				if (core_unlikely((v0 | v1 | v2) < 0)) {
					return -1;
				}

				// Decode with padding
				dest[0] = (uint8_t)((v0 << 2) | (v1 >> 4));
				if (decodedSize > 1) {
					dest[1] = (uint8_t)((v1 << 4) | (v2 >> 2));
				}

				return bytesWritten + decodedSize;
			}

			// Fast path: no padding - reverse lookup all 4 characters
			const int v0 = reverseLookup(e0);
			const int v1 = reverseLookup(e1);
			const int v2 = reverseLookup(e2);
			const int v3 = reverseLookup(e3);

			if (core_unlikely((v0 | v1 | v2 | v3) < 0)) {
				return -1;
			}

			// Decode the block
			dest[0] = (uint8_t)((v0 << 2) | (v1 >> 4));
			dest[1] = (uint8_t)((v1 << 4) | (v2 >> 2));
			dest[2] = (uint8_t)((v2 << 6) | v3);

			encoded += 4;
			dest += 3;
			bytesWritten += 3;
		}
	}

	// Slow path: not enough room for a full block, decode to cache
	if (bytesWritten < size) {
		if (_stream.eos()) {
			return bytesWritten;
		}

		const int decoded = decodeBlock(_stream, _readBuf);
		if (core_unlikely(decoded < 0)) {
			return -1;
		}

		_readBufSize = decoded;
		_readBufPos = 0;

		// Copy what we can to output
		while (_readBufSize > 0 && bytesWritten < size) {
			bytesPtr[bytesWritten++] = _readBuf[_readBufPos++];
			--_readBufSize;
		}
	}

	return bytesWritten;
}

bool Base64ReadStream::eos() const {
	return _stream.eos() && _readBufSize == 0;
}

} // namespace io
