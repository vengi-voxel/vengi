/**
 * @file
 */

#include "ZipWriteStream.h"
#include "core/StandardLib.h"
#include "core/private/miniz.h"
#include "core/Assert.h"

namespace io {

ZipWriteStream::ZipWriteStream(io::WriteStream &outStream, int level) : _outStream(outStream) {
	_stream = (mz_stream *)core_malloc(sizeof(*_stream));
	core_memset(_stream, 0, sizeof(*_stream));
	_stream->zalloc = Z_NULL;
	_stream->zfree = Z_NULL;
	mz_deflateInit(_stream, level);
}

ZipWriteStream::~ZipWriteStream() {
	ZipWriteStream::flush();
	const int retVal = mz_deflateEnd(_stream);
	core_free(_stream);
	core_assert(retVal == MZ_OK);
}

int ZipWriteStream::write(const void *buf, size_t size) {
	_stream->next_in = (unsigned char *)buf;
	_stream->avail_in = static_cast<unsigned int>(size);

	uint32_t writtenBytes = 0;
	while (_stream->avail_in > 0) {
		_stream->avail_out = sizeof(_out);
		_stream->next_out = _out;

		const int retVal = mz_deflate(_stream, MZ_NO_FLUSH);
		if (retVal != MZ_OK) {
			return -1;
		}

		const uint32_t written = sizeof(_out) - _stream->avail_out;
		if (written != 0u) {
			if (_outStream.write(_out, written) != (int)written) {
				return -1;
			}
			writtenBytes += written;
			_pos += written;
		}
	}
	return (int)writtenBytes;
}

bool ZipWriteStream::flush() {
	_stream->avail_in = 0;
	_stream->avail_out = sizeof(_out);
	_stream->next_out = _out;

	const int retVal = mz_deflate(_stream, MZ_FINISH);
	if (retVal == MZ_STREAM_ERROR) {
		return false;
	}

	size_t remaining = sizeof(_out) - _stream->avail_out;
	const int written = _outStream.write(_out, remaining);
	_pos += written;
	return written == (int)remaining;
}

} // namespace io
