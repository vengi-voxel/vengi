/**
 * @file
 */

#include "LZ4ReadStream.h"
#include "core/Log.h"
#include "core/StandardLib.h"
#include "io/BufferedReadWriteStream.h"

#include <lz4frame.h>

namespace io {

LZ4ReadStream::LZ4ReadStream(io::SeekableReadStream &readStream, int size) {
	BufferedReadWriteStream s(readStream, size <= 0 ? readStream.remaining() : size);
	size_t availableIn = (size_t)s.size();
	const uint8_t *src = s.getBuffer();

	LZ4F_dctx *dctx = nullptr;
	const LZ4F_errorCode_t err = LZ4F_createDecompressionContext(&dctx, LZ4F_VERSION);
	if (LZ4F_isError(err) || dctx == nullptr) {
		Log::error("LZ4: failed to create decompression context: %s", LZ4F_getErrorName(err));
		return;
	}

	size_t extractedBufferSize = availableIn * 4u;
	if (extractedBufferSize < 4096u) {
		extractedBufferSize = 4096u;
	}
	_extractedBuffer = (uint8_t *)core_malloc(extractedBufferSize);
	if (_extractedBuffer == nullptr) {
		Log::error("LZ4: failed to allocate output buffer");
		LZ4F_freeDecompressionContext(dctx);
		return;
	}

	size_t srcPos = 0;
	size_t totalOut = 0;
	for (;;) {
		size_t srcSize = availableIn - srcPos;
		size_t dstSize = extractedBufferSize - totalOut;
		const size_t srcSizeBefore = srcSize;
		const size_t result =
			LZ4F_decompress(dctx, _extractedBuffer + totalOut, &dstSize, src + srcPos, &srcSize, nullptr);
		srcPos += srcSize;
		totalOut += dstSize;
		if (LZ4F_isError(result)) {
			Log::error("LZ4: decompression failed: %s", LZ4F_getErrorName(result));
			LZ4F_freeDecompressionContext(dctx);
			core_free(_extractedBuffer);
			_extractedBuffer = nullptr;
			return;
		}
		if (result == 0) {
			break;
		}
		// Need more output space
		if (dstSize == 0 && srcSize == 0 && srcSizeBefore > 0) {
			extractedBufferSize <<= 1u;
			uint8_t *resized = (uint8_t *)core_realloc(_extractedBuffer, extractedBufferSize);
			if (resized == nullptr) {
				Log::error("LZ4: failed to grow output buffer");
				LZ4F_freeDecompressionContext(dctx);
				core_free(_extractedBuffer);
				_extractedBuffer = nullptr;
				return;
			}
			_extractedBuffer = resized;
			continue;
		}
		if (totalOut == extractedBufferSize) {
			extractedBufferSize <<= 1u;
			uint8_t *resized = (uint8_t *)core_realloc(_extractedBuffer, extractedBufferSize);
			if (resized == nullptr) {
				Log::error("LZ4: failed to grow output buffer");
				LZ4F_freeDecompressionContext(dctx);
				core_free(_extractedBuffer);
				_extractedBuffer = nullptr;
				return;
			}
			_extractedBuffer = resized;
			continue;
		}
		// Incomplete/truncated or invalid frame with no more input
		if (srcPos >= availableIn && srcSize == 0) {
			Log::error("LZ4: incomplete compressed stream");
			LZ4F_freeDecompressionContext(dctx);
			core_free(_extractedBuffer);
			_extractedBuffer = nullptr;
			return;
		}
	}

	LZ4F_freeDecompressionContext(dctx);
	if (totalOut > 0 && totalOut < extractedBufferSize) {
		uint8_t *shrunk = (uint8_t *)core_realloc(_extractedBuffer, totalOut);
		if (shrunk != nullptr) {
			_extractedBuffer = shrunk;
		}
	} else if (totalOut == 0) {
		core_free(_extractedBuffer);
		_extractedBuffer = nullptr;
	}
	_readStream = new MemoryReadStream(_extractedBuffer, totalOut);
}

LZ4ReadStream::~LZ4ReadStream() {
	delete _readStream;
	core_free(_extractedBuffer);
}

int64_t LZ4ReadStream::seek(int64_t position, int whence) {
	if (_readStream == nullptr) {
		return -1;
	}
	return _readStream->seek(position, whence);
}

int64_t LZ4ReadStream::size() const {
	if (_readStream == nullptr) {
		return 0;
	}
	return _readStream->size();
}

int64_t LZ4ReadStream::pos() const {
	if (_readStream == nullptr) {
		return 0;
	}
	return _readStream->pos();
}

int LZ4ReadStream::read(void *buf, size_t size) {
	if (_readStream == nullptr) {
		return -1;
	}
	return _readStream->read(buf, size);
}

} // namespace io
