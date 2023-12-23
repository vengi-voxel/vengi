/**
 * @file
 */

#include "Base64ReadStream.h"
#include "core/ArrayLength.h"
#include "io/Stream.h"

namespace io {

Base64ReadStream::Base64ReadStream(io::ReadStream &stream) : _stream(stream) {
	for (int i = 0; i < lengthof(LUT); ++i) {
		_reverseLookupTable[(uint8_t)LUT[i]] = i;
	}
}

int Base64ReadStream::read(void *buf, size_t size) {
	uint8_t *bytesPtr = (uint8_t *)buf;
	size_t bytesWritten = 0;
	while (bytesWritten < size) {
		// read from cache
		if (_readBufSize > 0) {
			bytesPtr[bytesWritten++] = _readBuf[_readBufPos++];
			--_readBufSize;
			continue;
		}

		_readBufPos = 0;

		uint8_t buf[4]{'\0', '\0', '\0', '\0'};
		for (int i = 0; i < 4; ++i) {
			uint8_t val = 0;
			if (_stream.readUInt8(val) != 0) {
				return -1;
			}
			if (val == '=') {
				return bytesWritten;
			}
			if (!validbyte(val)) {
				return -1;
			}
			buf[i] = _reverseLookupTable[val];
		}
		if (!decode(buf, _readBuf)) {
			return -1;
		}
		_readBufSize = 3;
		_readBufPos = 0;
	}
	return bytesWritten;
}

bool Base64ReadStream::eos() const {
	return _stream.eos() && _readBufSize == 0;
}

} // namespace io
