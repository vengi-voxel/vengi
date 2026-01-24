/**
 * @file
 */

#include "ZipWriteStream.h"
#include "core/StandardLib.h"
#include "core/collection/Buffer.h"
#if USE_LIBDEFLATE
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
struct ZipWriteLibDeflateState {
	libdeflate_compressor *compressor = nullptr;
	core::Buffer<uint8_t> inputBuffer;
	int level;
	bool rawDeflate;
};
#endif

ZipWriteStream::ZipWriteStream(io::WriteStream &outStream, int level, bool rawDeflate) : _outStream(outStream) {
#if USE_LIBDEFLATE
	ZipWriteLibDeflateState *state = new ZipWriteLibDeflateState();
	state->compressor = libdeflate_alloc_compressor(level);
	state->level = level;
	state->rawDeflate = rawDeflate;
	_stream = state;
#else
	_stream = (z_stream *)core_malloc(sizeof(z_stream));
	core_memset(((z_stream *)_stream), 0, sizeof(*((z_stream *)_stream)));
	((z_stream *)_stream)->zalloc = Z_NULL;
	((z_stream *)_stream)->zfree = Z_NULL;
	const int windowBits = rawDeflate ? -Z_DEFAULT_WINDOW_BITS : Z_DEFAULT_WINDOW_BITS;
	deflateInit2(((z_stream *)_stream), level, Z_DEFLATED, windowBits, 9, Z_DEFAULT_STRATEGY);
#endif
}

ZipWriteStream::~ZipWriteStream() {
	ZipWriteStream::flush();
#if USE_LIBDEFLATE
	ZipWriteLibDeflateState *state = (ZipWriteLibDeflateState *)_stream;
	libdeflate_free_compressor(state->compressor);
	delete state;
#else
	const int retVal = deflateEnd(((z_stream *)_stream));
	core_free(((z_stream *)_stream));
	core_assert(retVal == Z_OK);
	(void)retVal;
#endif
}

int ZipWriteStream::write(const void *buf, size_t size) {
#if USE_LIBDEFLATE
	ZipWriteLibDeflateState *state = (ZipWriteLibDeflateState *)_stream;
	const uint8_t *p = (const uint8_t *)buf;
	const size_t newSize = state->inputBuffer.size() + size;
	if (state->inputBuffer.capacity() < newSize) {
		state->inputBuffer.reserve(core_max(state->inputBuffer.capacity() * 2, newSize));
	}
	state->inputBuffer.append(p, size);
	return (int)size;
#else
	((z_stream *)_stream)->next_in = (const unsigned char *)buf;
	((z_stream *)_stream)->avail_in = static_cast<unsigned int>(size);

	uint32_t writtenBytes = 0;
	while (((z_stream *)_stream)->avail_in > 0) {
		((z_stream *)_stream)->avail_out = sizeof(_out);
		((z_stream *)_stream)->next_out = _out;

		const int retVal = deflate(((z_stream *)_stream), Z_NO_FLUSH);
		if (retVal != Z_OK) {
			return -1;
		}

		const uint32_t written = sizeof(_out) - ((z_stream *)_stream)->avail_out;
		if (written != 0u) {
			if (_outStream.write(_out, written) != (int)written) {
				return -1;
			}
			writtenBytes += written;
			_pos += written;
		}
	}
	return (int)writtenBytes;
#endif
}

bool ZipWriteStream::flush() {
#if USE_LIBDEFLATE
	ZipWriteLibDeflateState *state = (ZipWriteLibDeflateState *)_stream;
	if (state->inputBuffer.empty()) {
		return true;
	}
	size_t bound = 0;
	if (state->rawDeflate) {
		bound = libdeflate_deflate_compress_bound(state->compressor, state->inputBuffer.size());
	} else {
		bound = libdeflate_zlib_compress_bound(state->compressor, state->inputBuffer.size());
	}

	core::Buffer<uint8_t> outBuf;
	outBuf.resize(bound);
	size_t compressedSize = 0;
	if (state->rawDeflate) {
		compressedSize = libdeflate_deflate_compress(state->compressor, state->inputBuffer.data(),
													 state->inputBuffer.size(), outBuf.data(), outBuf.size());
	} else {
		compressedSize = libdeflate_zlib_compress(state->compressor, state->inputBuffer.data(),
												  state->inputBuffer.size(), outBuf.data(), outBuf.size());
	}

	if (compressedSize == 0) {
		return false;
	}

	if (_outStream.write(outBuf.data(), compressedSize) != (int)compressedSize) {
		return false;
	}
	_pos += compressedSize;
	state->inputBuffer.clear();
	return true;
#else
	((z_stream *)_stream)->avail_in = 0;
	((z_stream *)_stream)->avail_out = sizeof(_out);
	((z_stream *)_stream)->next_out = _out;

	const int retVal = deflate(((z_stream *)_stream), Z_FINISH);
	if (retVal == Z_STREAM_ERROR) {
		return false;
	}

	size_t remaining = sizeof(_out) - ((z_stream *)_stream)->avail_out;
	const int written = _outStream.write(_out, remaining);
	_pos += written;
	return written == (int)remaining;
#endif
}

} // namespace io
