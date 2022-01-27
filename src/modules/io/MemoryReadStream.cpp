/**
 * @file
 */

#include "MemoryReadStream.h"
#include "core/Assert.h"
#include "core/Log.h"
#include "core/StandardLib.h"

namespace io {

int MemoryReadStream::read(void *dataPtr, size_t dataSize) {
	if (_pos + (int64_t)dataSize > _size) {
		return -1;
	}
	core_memcpy(dataPtr, &_buf[_pos], dataSize);
	_pos += (int64_t)dataSize;
	return (int)dataSize;
}

int64_t MemoryReadStream::seek(int64_t position, int whence) {
	switch (whence) {
	case SEEK_SET:
		_pos = position;
		break;
	case SEEK_CUR:
		_pos += position;
		break;
	case SEEK_END:
		_pos = _size + position;
		break;
	default:
		return -1;
	}
	if (_pos < 0) {
		_pos = 0;
	} else if (_pos > _size) {
		_pos = _size;
	}
	return _pos;
}

} // namespace io
