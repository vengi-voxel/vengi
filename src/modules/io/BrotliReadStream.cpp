/**
 * @file
 */

#include "BrotliReadStream.h"
#include "core/Log.h"
#include "core/StandardLib.h"
#include "io/BufferedReadWriteStream.h"

#include <brotli/decode.h>

namespace io {

static void *brotliAlloc(void *, size_t size) {
	return core_malloc(size);
}

static void brotliFree(void *, void *address) {
	core_free(address);
}

BrotliReadStream::BrotliReadStream(io::SeekableReadStream &readStream, int size) {
	BufferedReadWriteStream s(readStream, size <= 0 ? readStream.remaining() : size);
	size_t availableIn = (size_t)s.size();
	const uint8_t *nextIn = s.getBuffer();

	BrotliDecoderState *state = BrotliDecoderCreateInstance(brotliAlloc, brotliFree, nullptr);
	if (state == nullptr) {
		Log::error("Brotli: failed to create decoder");
		return;
	}

	size_t extractedBufferSize = availableIn * 4u;
	if (extractedBufferSize < 4096u) {
		extractedBufferSize = 4096u;
	}
	_extractedBuffer = (uint8_t *)core_malloc(extractedBufferSize);
	if (_extractedBuffer == nullptr) {
		Log::error("Brotli: failed to allocate output buffer");
		BrotliDecoderDestroyInstance(state);
		return;
	}

	size_t availableOut = extractedBufferSize;
	uint8_t *nextOut = _extractedBuffer;
	size_t totalOut = 0;

	for (;;) {
		const BrotliDecoderResult result =
			BrotliDecoderDecompressStream(state, &availableIn, &nextIn, &availableOut, &nextOut, &totalOut);
		if (result == BROTLI_DECODER_RESULT_SUCCESS) {
			break;
		}
		if (result == BROTLI_DECODER_RESULT_NEEDS_MORE_OUTPUT) {
			const size_t used = extractedBufferSize - availableOut;
			extractedBufferSize <<= 1u;
			uint8_t *resized = (uint8_t *)core_realloc(_extractedBuffer, extractedBufferSize);
			if (resized == nullptr) {
				Log::error("Brotli: failed to grow output buffer");
				BrotliDecoderDestroyInstance(state);
				core_free(_extractedBuffer);
				_extractedBuffer = nullptr;
				return;
			}
			_extractedBuffer = resized;
			nextOut = _extractedBuffer + used;
			availableOut = extractedBufferSize - used;
			continue;
		}
		Log::error("Brotli: decompression failed: %s", BrotliDecoderErrorString(BrotliDecoderGetErrorCode(state)));
		BrotliDecoderDestroyInstance(state);
		core_free(_extractedBuffer);
		_extractedBuffer = nullptr;
		return;
	}

	BrotliDecoderDestroyInstance(state);
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

BrotliReadStream::~BrotliReadStream() {
	delete _readStream;
	core_free(_extractedBuffer);
}

int64_t BrotliReadStream::seek(int64_t position, int whence) {
	if (_readStream == nullptr) {
		return -1;
	}
	return _readStream->seek(position, whence);
}

int64_t BrotliReadStream::size() const {
	if (_readStream == nullptr) {
		return 0;
	}
	return _readStream->size();
}

int64_t BrotliReadStream::pos() const {
	if (_readStream == nullptr) {
		return 0;
	}
	return _readStream->pos();
}

int BrotliReadStream::read(void *buf, size_t size) {
	if (_readStream == nullptr) {
		return -1;
	}
	return _readStream->read(buf, size);
}

} // namespace io
