/**
 * @file
 */

#include "ZipReadStream.h"
#include "core/Log.h"
#include "core/StandardLib.h"
#include "core/miniz.h"

namespace io {

ZipReadStream::ZipReadStream(io::SeekableReadStream &readStream, int size)
	: _readStream(readStream), _size(size), _remaining(size) {
	_stream = (mz_stream *)core_malloc(sizeof(*_stream));
	core_memset(_stream, 0, sizeof(*_stream));
	_stream->zalloc = Z_NULL;
	_stream->zfree = Z_NULL;
	mz_inflateInit(_stream);
}

ZipReadStream::~ZipReadStream() {
	mz_inflateEnd(_stream);
	core_free(_stream);
}

bool ZipReadStream::eos() const {
	return _eos;
}

int64_t ZipReadStream::remaining() const {
	if (_size >= 0) {
		core_assert_msg(_remaining >= 0, "if size is given (%i), remaining should be >= 0 - but is %i", _size, _remaining);
		return core_min(_remaining, _readStream.remaining());
	}
	return _readStream.remaining();
}

int ZipReadStream::read(void *buf, size_t size) {
	uint8_t *targetPtr = (uint8_t *)buf;
	const size_t originalSize = size;
	while (size > 0) {
		if (_stream->avail_in == 0) {
			int64_t remainingSize = remaining();
			_stream->next_in = _buf;
			_stream->avail_in = (unsigned int)core_min(remainingSize, (int64_t)sizeof(_buf));
			if (remainingSize > 0) {
				_readStream.read(_buf, _stream->avail_in);
				if (_size >= 0) {
					_remaining -= (int)_stream->avail_in;
				}
			}
		}

		_stream->avail_out = (unsigned int)size;
		_stream->next_out = targetPtr;

		const int retval = mz_inflate(_stream, MZ_NO_FLUSH);
		switch (retval) {
		case MZ_STREAM_ERROR:
		case MZ_NEED_DICT:
		case MZ_DATA_ERROR:
		case MZ_MEM_ERROR:
			return -1;
		}

		const size_t outputSize = size - (size_t)_stream->avail_out;
		targetPtr += outputSize;
		core_assert(size >= outputSize);
		size -= outputSize;

		if (retval == MZ_STREAM_END) {
			_eos = true;
			if (size > 0) {
				// attempting to read past the end of the stream
				return -1;
			}
		}
	}
	return (int)originalSize;
}

} // namespace io