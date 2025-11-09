/**
 * @file
 */

#include "Base64WriteStream.h"
#include "io/Stream.h"

namespace io {

Base64WriteStream::Base64WriteStream(io::WriteStream &stream) : _stream(stream) {
}

Base64WriteStream::~Base64WriteStream() {
	flush();
}

int Base64WriteStream::write(const void *buf, size_t size) {
	int written = 0;
	const uint8_t *bytesPtr = (const uint8_t *)buf;
	size_t total = size;
	_stream.reserve((size + 2) / 3 * 4);

	// If there are leftover bytes from previous write, fill _buf first
	if (_bytes > 0) {
		while (_bytes < 3 && total > 0) {
			_buf[_bytes++] = *bytesPtr++;
			total--;
		}
		if (_bytes == 3) {
			if (!encode(_buf, _stream, 4)) {
				return -1;
			}
			written += 4;
			_bytes = 0;
		}
	}

	// Process as many full 3-byte blocks as possible in large batches to minimize virtual calls
	size_t fullBlocks = total / 3;
	const size_t maxBlocksPerBatch = 1024; // produces 4096 bytes per batch on stack
	size_t processedBlocks = 0;
	while (processedBlocks < fullBlocks) {
		const size_t batchBlocks =
			(fullBlocks - processedBlocks > maxBlocksPerBatch) ? maxBlocksPerBatch : (fullBlocks - processedBlocks);
		const uint8_t *src = bytesPtr + processedBlocks * 3;
		char outbuf[maxBlocksPerBatch * 4];
		char *dst = outbuf;
		for (size_t i = 0; i < batchBlocks; ++i) {
			const uint8_t s0 = src[i * 3 + 0];
			const uint8_t s1 = src[i * 3 + 1];
			const uint8_t s2 = src[i * 3 + 2];
			const uint8_t d0 = (s0 & 0xfc) >> 2;
			const uint8_t d1 = ((s0 & 0x03) << 4) | ((s1 & 0xf0) >> 4);
			const uint8_t d2 = ((s1 & 0x0f) << 2) | ((s2 & 0xc0) >> 6);
			const uint8_t d3 = (s2 & 0x3f);
			dst[0] = LUT[d0];
			dst[1] = LUT[d1];
			dst[2] = LUT[d2];
			dst[3] = LUT[d3];
			dst += 4;
		}
		const size_t outBytes = batchBlocks * 4;
		const int w = _stream.write(outbuf, outBytes);
		if (w == -1) {
			return -1;
		}
		written += w;
		processedBlocks += batchBlocks;
	}

	// Store remaining bytes in _buf
	size_t remainder = total % 3;
	if (remainder > 0) {
		const uint8_t *remPtr = bytesPtr + fullBlocks * 3;
		for (size_t i = 0; i < remainder; ++i) {
			_buf[_bytes++] = remPtr[i];
		}
	}

	return written + _bytes;
}

bool Base64WriteStream::flush() {
	// handle remaining bytes
	if (_bytes) {
		// pad with zeros
		for (int i = _bytes; i < 3; ++i) {
			_buf[i] = '\0';
		}
		// produce up to 3 output chars for data, then '=' padding
		const uint8_t s0 = _buf[0];
		const uint8_t s1 = _buf[1];
		const uint8_t s2 = _buf[2];
		const uint8_t d0 = (s0 & 0xfc) >> 2;
		const uint8_t d1 = ((s0 & 0x03) << 4) | ((s1 & 0xf0) >> 4);
		const uint8_t d2 = ((s1 & 0x0f) << 2) | ((s2 & 0xc0) >> 6);
		const uint8_t d3 = (s2 & 0x3f);
		char data[4];
		data[0] = LUT[d0];
		data[1] = LUT[d1];
		data[2] = LUT[d2];
		data[3] = LUT[d3];
		const int dataBytes = _bytes + 1; // mapping: 1->2, 2->3
		if (_stream.write(data, (size_t)dataBytes) == -1) {
			return false;
		}
		const int padCount = 3 - _bytes; // 1->2 pads, 2->1 pad
		if (padCount > 0) {
			const char pad[3] = {'=', '=', '='};
			if (_stream.write(pad, (size_t)padCount) == -1) {
				return false;
			}
		}
		_bytes = 0;
	}
	return true;
}

} // namespace io
