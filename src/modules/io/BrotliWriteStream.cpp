/**
 * @file
 */

#include "BrotliWriteStream.h"
#include "core/Log.h"
#include "core/StandardLib.h"

#include <brotli/encode.h>

namespace io {

static void *brotliEncoderAlloc(void *, size_t size) {
	return core_malloc(size);
}

static void brotliEncoderFree(void *, void *address) {
	core_free(address);
}

BrotliWriteStream::BrotliWriteStream(io::WriteStream &outStream, int quality) : _outStream(outStream) {
	BrotliEncoderState *state = BrotliEncoderCreateInstance(brotliEncoderAlloc, brotliEncoderFree, nullptr);
	if (state == nullptr) {
		Log::error("Brotli: failed to create encoder");
		return;
	}
	if (quality < BROTLI_MIN_QUALITY) {
		quality = BROTLI_MIN_QUALITY;
	}
	if (quality > BROTLI_MAX_QUALITY) {
		quality = BROTLI_MAX_QUALITY;
	}
	BrotliEncoderSetParameter(state, BROTLI_PARAM_QUALITY, (uint32_t)quality);
	_state = state;
}

BrotliWriteStream::~BrotliWriteStream() {
	BrotliWriteStream::flush();
	if (_state != nullptr) {
		BrotliEncoderDestroyInstance((BrotliEncoderState *)_state);
		_state = nullptr;
	}
}

bool BrotliWriteStream::compress(const uint8_t *data, size_t size, int operation) {
	if (_state == nullptr) {
		return false;
	}
	BrotliEncoderState *state = (BrotliEncoderState *)_state;
	size_t availableIn = size;
	const uint8_t *nextIn = data;
	for (;;) {
		size_t availableOut = sizeof(_out);
		uint8_t *nextOut = _out;
		if (!BrotliEncoderCompressStream(state, (BrotliEncoderOperation)operation, &availableIn, &nextIn, &availableOut,
										 &nextOut, nullptr)) {
			Log::error("Brotli: compression failed");
			return false;
		}
		const size_t produced = sizeof(_out) - availableOut;
		if (produced > 0) {
			if (_outStream.write(_out, produced) != (int)produced) {
				Log::error("Brotli: failed to write compressed data");
				return false;
			}
			_pos += (int64_t)produced;
		}
		if (operation == BROTLI_OPERATION_PROCESS) {
			if (availableIn == 0) {
				return true;
			}
			continue;
		}
		if (BrotliEncoderIsFinished(state)) {
			return true;
		}
	}
}

int BrotliWriteStream::write(const void *buf, size_t size) {
	if (_finished) {
		return -1;
	}
	if (size == 0) {
		return 0;
	}
	if (!compress((const uint8_t *)buf, size, BROTLI_OPERATION_PROCESS)) {
		return -1;
	}
	return (int)size;
}

bool BrotliWriteStream::flush() {
	if (_finished) {
		return true;
	}
	_finished = true;
	return compress(nullptr, 0, BROTLI_OPERATION_FINISH);
}

} // namespace io
