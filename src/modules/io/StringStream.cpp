/**
 * @file
 */

#include "StringStream.h"

namespace io {

StringReadStream::StringReadStream(SeekableReadStream &stream) : _stream(stream) {
}

StringReadStream::~StringReadStream() {
}

int64_t StringReadStream::size() const {
	return _stream.size();
}

int64_t StringReadStream::pos() const {
	return _stream.pos();
}

int StringReadStream::read(void *dataPtr, size_t dataSize) {
	return _stream.read(dataPtr, dataSize);
}

int64_t StringReadStream::seek(int64_t position, int whence) {
	return _stream.seek(position, whence);
}

core::String StringReadStream::readAll() {
	core::String str;
	str.reserve(remaining());
	int8_t c;
	while (readInt8(c) != -1) {
		if (c != '\0') {
			str += c;
		}
	}
	return str;
}

} // namespace io
