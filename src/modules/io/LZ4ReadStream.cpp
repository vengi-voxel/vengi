/**
 * @file
 */

#include "LZ4ReadStream.h"
#include "core/Log.h"
#include "core/StandardLib.h"
#include "io/BufferedReadWriteStream.h"

#include <limits.h>
#include <lz4frame.h>

namespace io {

LZ4FrameReader::LZ4FrameReader(const uint8_t *data, size_t size) : _input(data), _remaining(size) {
	const size_t result = LZ4F_createDecompressionContext(&_ctx, LZ4F_VERSION);
	if (LZ4F_isError(result) || _ctx == nullptr) {
		Log::error("LZ4: failed to create decompression context: %s", LZ4F_getErrorName(result));
		_failed = true;
	}
}

LZ4FrameReader::~LZ4FrameReader() {
	LZ4F_freeDecompressionContext(_ctx);
}

int LZ4FrameReader::read(void *buf, size_t size) {
	if (_failed || size > INT_MAX) {
		return -1;
	}
	size_t produced = 0;
	while (!_finished && produced < size) {
		size_t srcSize = _remaining;
		size_t dstSize = size - produced;
		const size_t result = LZ4F_decompress(_ctx, (uint8_t *)buf + produced, &dstSize, _input, &srcSize, nullptr);
		if (LZ4F_isError(result)) {
			Log::error("LZ4: decompression failed: %s", LZ4F_getErrorName(result));
			_failed = true;
			return -1;
		}
		if (result != 0 && srcSize == 0 && dstSize == 0) {
			Log::error("LZ4: incomplete compressed stream");
			_failed = true;
			return -1;
		}
		_remaining -= srcSize;
		if (srcSize > 0) {
			_input += srcSize;
		}
		produced += dstSize;
		_finished = result == 0;
	}
	return (int)produced;
}

bool LZ4FrameReader::skip(size_t size) {
	uint8_t discard[16 * 1024];
	while (size > 0) {
		const size_t bytes = core_min(size, sizeof(discard));
		if (read(discard, bytes) != (int)bytes) {
			return false;
		}
		size -= bytes;
	}
	return true;
}

LZ4ReadStream::LZ4ReadStream(io::SeekableReadStream &readStream, int size) {
	BufferedReadWriteStream compressed(readStream, size <= 0 ? readStream.remaining() : size);
	LZ4FrameReader reader(compressed.getBuffer(), (size_t)compressed.size());

	size_t capacity = 4096u;
	_extractedBuffer = (uint8_t *)core_malloc(capacity);
	if (_extractedBuffer == nullptr) {
		Log::error("LZ4: failed to allocate output buffer");
		return;
	}
	size_t totalOut = 0;
	while (!reader.eos()) {
		if (totalOut == capacity) {
			if (capacity > SIZE_MAX / 2u) {
				return;
			}
			capacity *= 2u;
			uint8_t *resized = (uint8_t *)core_realloc(_extractedBuffer, capacity);
			if (resized == nullptr) {
				Log::error("LZ4: failed to grow output buffer");
				return;
			}
			_extractedBuffer = resized;
		}
		const int bytes = reader.read(_extractedBuffer + totalOut, core_min(capacity - totalOut, (size_t)INT_MAX));
		if (bytes < 0) {
			return;
		}
		totalOut += bytes;
	}
	if (totalOut == 0) {
		core_free(_extractedBuffer);
		_extractedBuffer = nullptr;
	} else if (totalOut < capacity) {
		if (uint8_t *shrunk = (uint8_t *)core_realloc(_extractedBuffer, totalOut)) {
			_extractedBuffer = shrunk;
		}
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
