/**
 * @file
 */

#include "ZipReadStream.h"
#include "core/Log.h"
#include "core/StandardLib.h"
#include "engine-config.h" // USE_ZLIB
#if USE_ZLIB
#define ZLIB_CONST
#ifndef Z_DEFAULT_WINDOW_BITS
#define Z_DEFAULT_WINDOW_BITS 15
#endif
#include <zlib.h>
#else
#define MINIZ_NO_STDIO
#include "io/external/miniz.h"
#endif
#include "core/Assert.h"

namespace io {

ZipReadStream::ZipReadStream(io::SeekableReadStream &readStream, int size)
	: _readStream(readStream), _size(size), _remaining(size) {
	_stream = (z_stream *)core_malloc(sizeof(z_stream));
	core_memset(((z_stream*)_stream), 0, sizeof(z_stream));
	((z_stream*)_stream)->zalloc = Z_NULL;
	((z_stream*)_stream)->zfree = Z_NULL;
	uint8_t gzipHeader[2];
	if (readStream.readUInt8(gzipHeader[0]) == -1) {
		_err = true;
	}
	if (readStream.readUInt8(gzipHeader[1]) == -1) {
		_err = true;
	}

	int windowBits = 0;
	if (gzipHeader[0] == 0x1F && gzipHeader[1] == 0x8B) {
		// gzip
		windowBits = -Z_DEFAULT_WINDOW_BITS;
		// gzip header is 10 bytes long
		readStream.skip(8);
	} else if ((gzipHeader[0] & 0x0F) == Z_DEFLATED &&							// Compression method is DEFLATE
			   ((gzipHeader[0] >> 4) >= 7 && (gzipHeader[0] >> 4) <= 15) && // Valid window size
			   ((gzipHeader[0] << 8 | gzipHeader[1]) % 31 == 0)) {
		// zlib
		Log::debug("detected zlib");
		windowBits = Z_DEFAULT_WINDOW_BITS;
		readStream.seek(-2, SEEK_CUR);
	} else {
		// raw deflate
		Log::debug("Detected raw deflate");
		windowBits = -Z_DEFAULT_WINDOW_BITS;
		readStream.seek(-2, SEEK_CUR);
	}
	if (inflateInit2(((z_stream*)_stream), windowBits) != Z_OK) {
		Log::error("Failed to initialize zip stream");
		_err = true;
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
			_err = true;
			return -1;
		}
	}
	return delta;
}

int ZipReadStream::read(void *buf, size_t size) {
	if (_eos) {
		return 0;
	}
	uint8_t *targetPtr = (uint8_t *)buf;
	const size_t originalSize = size;
	z_stream* stream = (z_stream*)_stream;
	size_t readCnt = 0;
	while (size > 0) {
		if (stream->avail_in == 0) {
			int64_t remainingSize = remaining();
			stream->next_in = _buf;
			stream->avail_in = (unsigned int)core_min(remainingSize, (int64_t)sizeof(_buf));
			if (remainingSize > 0) {
				const int bytes = _readStream.read(_buf, stream->avail_in);
				if (bytes == -1) {
					Log::debug("Failed to read from parent stream");
					_err = true;
					return -1;
				}
				if (_size >= 0) {
					_remaining -= bytes;
				}
			}
		}

		stream->avail_out = (unsigned int)size;
		stream->next_out = targetPtr;

		const int retval = inflate(stream, Z_NO_FLUSH);
		switch (retval) {
		case Z_OK:
		case Z_STREAM_END:
			break;
		default:
			_err = true;
			Log::debug("error while reading the stream: '%s'", zError(retval));
			return -1;
		}

		const size_t outputSize = size - (size_t)stream->avail_out;
		targetPtr += outputSize;
		core_assert(size >= outputSize);
		size -= outputSize;
		readCnt += outputSize;

		if (retval == Z_STREAM_END) {
			_eos = true;
			return readCnt;
		}
	}
	return (int)originalSize;
}

} // namespace io
