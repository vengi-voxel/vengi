/**
 * @file
 */

#include "ZipReadStream.h"
#include "core/Log.h"
#include "core/StandardLib.h"
#define MINIZ_NO_STDIO
#include "io/external/miniz.h"
#include "core/Assert.h"

namespace io {

ZipReadStream::ZipReadStream(io::SeekableReadStream &readStream, int size)
	: _readStream(readStream), _size(size), _remaining(size) {
	_stream = (z_stream *)core_malloc(sizeof(z_stream));
	core_memset(((z_stream*)_stream), 0, sizeof(z_stream));
	((z_stream*)_stream)->zalloc = Z_NULL;
	((z_stream*)_stream)->zfree = Z_NULL;
	uint8_t gzipHeader[2];
	readStream.readUInt8(gzipHeader[0]);
	readStream.readUInt8(gzipHeader[1]);
	if (gzipHeader[0] == 0x1F && gzipHeader[1] == 0x8B) {
		readStream.skip(8); // gzip header is 10 bytes
		inflateInit2(((z_stream*)_stream), -Z_DEFAULT_WINDOW_BITS);
	} else {
		readStream.seek(-2, SEEK_CUR);
		if (inflateInit(((z_stream*)_stream)) != Z_OK) {
			Log::error("Failed to initialize zip stream");
		}
	}
}

ZipReadStream::~ZipReadStream() {
	inflateEnd(((z_stream*)_stream));
	core_free(((z_stream*)_stream));
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

int64_t ZipReadStream::skip(int64_t delta) {
	for (int64_t i = 0; i < delta; ++i) {
		uint8_t b = 0;
		if (readUInt8(b) == -1) {
			return -1;
		}
	}
	return delta;
}

int ZipReadStream::read(void *buf, size_t size) {
	uint8_t *targetPtr = (uint8_t *)buf;
	const size_t originalSize = size;
	while (size > 0) {
		if (((z_stream*)_stream)->avail_in == 0) {
			int64_t remainingSize = remaining();
			((z_stream*)_stream)->next_in = _buf;
			((z_stream*)_stream)->avail_in = (unsigned int)core_min(remainingSize, (int64_t)sizeof(_buf));
			if (remainingSize > 0) {
				const int bytes = _readStream.read(_buf, ((z_stream*)_stream)->avail_in);
				if (bytes == -1) {
					Log::debug("Failed to read from parent stream");
					return -1;
				}
				if (_size >= 0) {
					_remaining -= bytes;
				}
			}
		}

		((z_stream*)_stream)->avail_out = (unsigned int)size;
		((z_stream*)_stream)->next_out = targetPtr;

		const int retval = inflate(((z_stream*)_stream), Z_NO_FLUSH);
		switch (retval) {
		case Z_OK:
		case Z_STREAM_END:
			break;
		default:
			Log::debug("error while reading the stream: '%s'", zError(retval));
			return -1;
		}

		const size_t outputSize = size - (size_t)((z_stream*)_stream)->avail_out;
		targetPtr += outputSize;
		core_assert(size >= outputSize);
		size -= outputSize;

		if (retval == Z_STREAM_END) {
			_eos = true;
			if (size > 0) {
				Log::debug("attempting to read past the end of the stream");
				return -1;
			}
		}
	}
	return (int)originalSize;
}

} // namespace io
