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
#include "io/external/miniz.h"
#endif
#include "core/Assert.h"

namespace io {

bool ZipReadStream::isZipStream(io::SeekableReadStream &readStream) {
	int64_t pos = readStream.pos();
	ZipReadStream s(readStream);
	if (s.err()) {
		readStream.seek(pos);
		return false;
	}
	uint32_t val;
	int retVal = s.readUInt32(val);
	readStream.seek(pos);
	return retVal == 0;
}

ZipReadStream::ZipReadStream(io::ReadStream &readStream, int size, CompressionType type)
	: _readStream(readStream), _size(size), _remaining(size) {
	_stream = (z_stream *)core_malloc(sizeof(z_stream));
	core_memset(((z_stream *)_stream), 0, sizeof(z_stream));
	((z_stream *)_stream)->zalloc = Z_NULL;
	((z_stream *)_stream)->zfree = Z_NULL;

	int windowBits = 0;
	switch (type) {
	case CompressionType::Deflate:
	case CompressionType::Gzip:
		windowBits = -Z_DEFAULT_WINDOW_BITS;
		readStream.skipDelta(10); // skip gzip header
		break;
	case CompressionType::Zlib:
		windowBits = Z_DEFAULT_WINDOW_BITS;
		break;
	default:
		_err = true;
		break;
	}
	if (inflateInit2(((z_stream *)_stream), windowBits) != Z_OK) {
		Log::error("Failed to initialize zip stream");
		_err = true;
	}
}

ZipReadStream::ZipReadStream(io::SeekableReadStream &readStream, int size)
	: _readStream(readStream), _size(size), _remaining(size) {
	_stream = (z_stream *)core_malloc(sizeof(z_stream));
	core_memset(((z_stream *)_stream), 0, sizeof(z_stream));
	((z_stream *)_stream)->zalloc = Z_NULL;
	((z_stream *)_stream)->zfree = Z_NULL;

	if (_remaining < 0 || readStream.remaining() < _remaining) {
		_remaining = (int)readStream.remaining();
	}
	const int64_t curPos = readStream.pos();
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
		readStream.seek(-4, SEEK_END);
		uint32_t isize = 0u;
		readStream.readUInt32(isize);
		_uncompressedSize = isize;
		Log::debug("detected gzip with uncompressed size %d", _uncompressedSize);
		readStream.seek(curPos, SEEK_SET);
		// gzip header is 10 bytes long
		readStream.skip(10);
	} else if ((gzipHeader[0] & 0x0F) == Z_DEFLATED &&						// Compression method is DEFLATE
			   ((gzipHeader[0] >> 4) >= 7 && (gzipHeader[0] >> 4) <= 15) && // Valid window size
			   ((gzipHeader[0] << 8 | gzipHeader[1]) % 31 == 0)) {
		// zlib
		Log::debug("detected zlib");
		windowBits = Z_DEFAULT_WINDOW_BITS;
		readStream.seek(curPos, SEEK_SET);
	} else {
		// raw deflate
		Log::debug("Detected raw deflate");
		windowBits = -Z_DEFAULT_WINDOW_BITS;
		readStream.seek(curPos, SEEK_SET);
	}
	if (inflateInit2(((z_stream *)_stream), windowBits) != Z_OK) {
		Log::error("Failed to initialize zip stream");
		_err = true;
	}
}

ZipReadStream::~ZipReadStream() {
	inflateEnd(((z_stream *)_stream));
	core_free(((z_stream *)_stream));
}

bool ZipReadStream::eos() const {
	return _eos;
}

int ZipReadStream::uncompressedSize() const {
	return _uncompressedSize;
}

int64_t ZipReadStream::remaining() const {
	if (_size >= 0) {
		core_assert_msg(_remaining >= 0, "if size is given (%i), remaining should be >= 0 - but is %i", _size,
						_remaining);
	}
	return _remaining;
}

int64_t ZipReadStream::skip(int64_t delta) {
	int64_t bytesSkipped = 0;
	uint8_t tempBuffer[1024];
	while (bytesSkipped < delta) {
		int64_t chunk = core_min(delta - bytesSkipped, (int64_t)sizeof(tempBuffer));
		if (read(tempBuffer, chunk) < chunk) {
			_err = true;
			return -1;
		}
		bytesSkipped += chunk;
	}
	return bytesSkipped;
}

int ZipReadStream::read(void *buf, size_t size) {
	if (_err) {
		return -1;
	}
	if (_eos) {
		return 0;
	}
	uint8_t *targetPtr = (uint8_t *)buf;
	z_stream *stream = (z_stream *)_stream;
	size_t readCnt = 0;
	while (size > 0) {
		if (stream->avail_in == 0) {
			int64_t remainingSize = remaining();
			if (remainingSize < 0) {
				_err = true;
				return readCnt;
			}
			stream->next_in = _buf;
			stream->avail_in = (unsigned int)core_min(remainingSize, (int64_t)sizeof(_buf));
			if (remainingSize > 0) {
				const int bytes = _readStream.read(_buf, stream->avail_in);
				if (bytes == -1) {
					Log::debug("Failed to read from parent stream");
					_err = true;
					return -1;
				}
				_remaining -= bytes;
				stream->avail_in = bytes;
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
			return (int)readCnt;
		}
	}
	return (int)readCnt;
}

} // namespace io
