/**
 * @file
 */

#include "Base64WriteStream.h"
#include "core/ArrayLength.h"
#include "io/Base64.h"
#include "io/Stream.h"

namespace io {

Base64WriteStream::Base64WriteStream(io::WriteStream &stream) : _stream(stream) {
}

Base64WriteStream::~Base64WriteStream() {
	flush();
}

int Base64WriteStream::write(const void *buf, size_t size) {
	uint8_t dest[4];
	int written = 0;
	const uint8_t *bytesPtr = (const uint8_t *)buf;
	_stream.reserve((size + 2) / 3 * 4);
	for (size_t i = 0; i < size; ++i) {
		uint8_t val = bytesPtr[i];
		_buf[_bytes++] = val;
		if (_bytes == 3) {
			if (!encode(_buf, _stream, lengthof(dest))) {
				return -1;
			}
			written += lengthof(dest);
			_bytes = 0;
		}
	}

	if (written == 0) {
		return size;
	}

	return written + _bytes;
}

bool Base64WriteStream::flush() {
	// handle remaining bytes
	if (_bytes) {
		for (int i = _bytes; i < 3; ++i) {
			_buf[i] = '\0';
		}
		if (!encode(_buf, _stream, _bytes + 1)) {
			return false;
		}
		while (_bytes++ < 3) {
			if (!_stream.writeUInt8('=')) {
				return false;
			}
		}
		_bytes = 0;
	}
	return true;
}

} // namespace io
