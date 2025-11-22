/**
 * @file
 */

#include "ZipReadStream.h"
#include "core/Log.h"
#include "core/StandardLib.h"
#include "engine-config.h" // USE_ZLIB, USE_LIBDEFLATE
#if USE_LIBDEFLATE
#include "core/collection/Buffer.h"
#include <libdeflate.h>
#elif USE_ZLIB
#define ZLIB_CONST
#ifndef Z_DEFAULT_WINDOW_BITS
#define Z_DEFAULT_WINDOW_BITS 15
#endif
#include <zlib.h>
#else // USE_ZLIB
#include "io/external/miniz.h"
#endif
#include "core/Assert.h"

namespace io {

#if USE_LIBDEFLATE
struct ZipReadLibDeflateState {
	libdeflate_decompressor *decompressor = nullptr;
	core::Buffer<uint8_t> uncompressedData;
	size_t uncompressedPos = 0;
	bool decompressed = false;
	CompressionType type = CompressionType::Deflate;
	int64_t compressedSize = -1;
};

#ifndef Z_DEFLATED
#define Z_DEFLATED 8
#endif
#endif

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
#if USE_LIBDEFLATE
	ZipReadLibDeflateState *state = new ZipReadLibDeflateState();
	state->decompressor = libdeflate_alloc_decompressor();
	state->type = type;
	state->compressedSize = size;
	_stream = state;
#else
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
#endif
}

ZipReadStream::ZipReadStream(io::SeekableReadStream &readStream, int size)
	: _readStream(readStream), _size(size), _remaining(size) {

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

#if USE_LIBDEFLATE
	ZipReadLibDeflateState *state = new ZipReadLibDeflateState();
	state->decompressor = libdeflate_alloc_decompressor();
	_stream = state;

	CompressionType type = CompressionType::Deflate;
	if (gzipHeader[0] == 0x1F && gzipHeader[1] == 0x8B) {
		type = CompressionType::Gzip;
		readStream.seek(-4, SEEK_END);
		uint32_t isize = 0u;
		readStream.readUInt32(isize);
		_uncompressedSize = isize;
		readStream.seek(curPos, SEEK_SET);
	} else if ((gzipHeader[0] & 0x0F) == Z_DEFLATED && ((gzipHeader[0] >> 4) >= 7 && (gzipHeader[0] >> 4) <= 15) &&
			   ((gzipHeader[0] << 8 | gzipHeader[1]) % 31 == 0)) {
		type = CompressionType::Zlib;
		readStream.seek(curPos, SEEK_SET);
	} else {
		type = CompressionType::Deflate;
		readStream.seek(curPos, SEEK_SET);
	}
	state->type = type;

	int64_t readSize = size;
	if (readSize == -1 || readStream.remaining() < readSize) {
		readSize = readStream.remaining();
	}
	state->compressedSize = readSize;
#else
	_stream = (z_stream *)core_malloc(sizeof(z_stream));
	core_memset(((z_stream *)_stream), 0, sizeof(z_stream));
	((z_stream *)_stream)->zalloc = Z_NULL;
	((z_stream *)_stream)->zfree = Z_NULL;

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
#endif
}

ZipReadStream::~ZipReadStream() {
#if USE_LIBDEFLATE
	ZipReadLibDeflateState *state = (ZipReadLibDeflateState *)_stream;
	libdeflate_free_decompressor(state->decompressor);
	delete state;
#else
	inflateEnd(((z_stream *)_stream));
	core_free(((z_stream *)_stream));
#endif
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
#if USE_LIBDEFLATE
	ZipReadLibDeflateState *state = (ZipReadLibDeflateState *)_stream;
	if (!state->decompressed) {
		state->decompressed = true;
		core::Buffer<uint8_t> compressedData;
		if (state->compressedSize > 0) {
			compressedData.resize(state->compressedSize);
			if (_readStream.read(compressedData.data(), state->compressedSize) != state->compressedSize) {
				_err = true;
				return -1;
			}
		} else if (state->compressedSize == -1) {
			uint8_t readBuf[4096];
			while (true) {
				int read = _readStream.read(readBuf, sizeof(readBuf));
				if (read < 0) {
					_err = true;
					return -1;
				}
				if (read == 0) {
					break;
				}
				const size_t newSize = compressedData.size() + read;
				if (compressedData.capacity() < newSize) {
					compressedData.reserve(core_max(compressedData.capacity() * 2, newSize));
				}
				compressedData.append(readBuf, read);
			}
		} else {
			_err = true;
			return -1;
		}

		size_t outSize = _uncompressedSize > 0 ? _uncompressedSize : (compressedData.size() * 4);
		if (outSize == 0) {
			outSize = 1024 * 1024;
		}

		while (true) {
			state->uncompressedData.resize(outSize);
			size_t actualOutSize = 0;
			libdeflate_result res;
			if (state->type == CompressionType::Gzip) {
				res = libdeflate_gzip_decompress(state->decompressor, compressedData.data(), compressedData.size(),
												 state->uncompressedData.data(), outSize, &actualOutSize);
			} else if (state->type == CompressionType::Zlib) {
				res = libdeflate_zlib_decompress(state->decompressor, compressedData.data(), compressedData.size(),
												 state->uncompressedData.data(), outSize, &actualOutSize);
			} else {
				res = libdeflate_deflate_decompress(state->decompressor, compressedData.data(), compressedData.size(),
													state->uncompressedData.data(), outSize, &actualOutSize);
			}

			if (res == LIBDEFLATE_SUCCESS) {
				state->uncompressedData.resize(actualOutSize);
				_uncompressedSize = (int)actualOutSize;
				_remaining = 0;
				break;
			} else if (res == LIBDEFLATE_INSUFFICIENT_SPACE) {
				outSize *= 2;
			} else {
				_err = true;
				Log::error("Failed to decompress zip stream");
				return -1;
			}
		}
	}

	size_t avail = state->uncompressedData.size() - state->uncompressedPos;
	size_t toRead = core_min(size, avail);
	if (toRead > 0) {
		core_memcpy(buf, state->uncompressedData.data() + state->uncompressedPos, toRead);
		state->uncompressedPos += toRead;
	}
	if (state->uncompressedPos >= state->uncompressedData.size()) {
		_eos = true;
	}
	return (int)toRead;
#else
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
#endif
}

} // namespace io
