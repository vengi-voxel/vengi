/**
 * @file
 */

#include "LZAVWriteStream.h"
#include "core/Log.h"
#include "core/StandardLib.h"

#define LZAV_MALLOC core_malloc
#define LZAV_FREE core_free

#include "external/lzav.h"

namespace io {

LZAVWriteStream::LZAVWriteStream(io::WriteStream &outStream, int level) : _outStream(outStream) {
	// LZAV doesn't have a streaming compression API or context structure
	// We'll buffer data and compress on flush
	// _stream is kept nullptr for consistency with header
	_stream = nullptr;
	Log::debug("LZAVWriteStream created successfully with level %d", level);
}

LZAVWriteStream::~LZAVWriteStream() {
	LZAVWriteStream::flush();
	if (_stream != nullptr) {
		core_free(_stream);
		_stream = nullptr;
	}
}

int LZAVWriteStream::write(const void *buf, size_t size) {
	if (size == 0) {
		return 0;
	}

	if (_finalized) {
		Log::error("Cannot write to finalized LZAV stream");
		return -1;
	}

	// LZAV doesn't support streaming compression, so we buffer all data
	// If buffer is too small, we need to use dynamic allocation
	const uint8_t *inputBuf = (const uint8_t *)buf;

	// Check if we need to switch to dynamic allocation
	if (_pos + size > sizeof(_out)) {
		// Need to use heap-allocated buffer
		if (_stream == nullptr) {
			// First time switching to heap - copy existing data
			const size_t newSize = _pos + size;
			_stream = core_malloc(newSize);
			if (_stream == nullptr) {
				Log::error("Failed to allocate dynamic buffer");
				return -1;
			}
			if (_pos > 0) {
				core_memcpy(_stream, _out, _pos);
			}
		} else {
			// Already using heap, need to grow it
			const size_t newSize = _pos + size;
			void *newBuf = core_realloc(_stream, newSize);
			if (newBuf == nullptr) {
				Log::error("Failed to reallocate dynamic buffer");
				return -1;
			}
			_stream = newBuf;
		}

		// Copy new data to heap buffer
		core_memcpy((uint8_t *)_stream + _pos, inputBuf, size);
		_pos += size;
	} else {
		// Data fits in static buffer
		core_memcpy(_out + _pos, inputBuf, size);
		_pos += size;
	}

	return (int)size;
}

bool LZAVWriteStream::flush() {
	if (_finalized) {
		return _outStream.flush();
	}

	if (_pos == 0) {
		// Nothing to flush
		_finalized = true;
		return _outStream.flush();
	}

	// Determine source buffer
	const uint8_t *srcBuf = _stream != nullptr ? (const uint8_t *)_stream : _out;
	const int srcSize = (int)_pos;
	const int maxCompressedSize = lzav_compress_bound(srcSize);

	uint8_t *compBuf = (uint8_t *)core_malloc(maxCompressedSize);
	if (compBuf == nullptr) {
		Log::error("Failed to allocate compression buffer");
		return false;
	}

	// Compress with default settings
	const int compressedSize = lzav_compress_default(srcBuf, compBuf, srcSize, maxCompressedSize);

	if (compressedSize <= 0) {
		Log::error("LZAV compression failed");
		core_free(compBuf);
		return false;
	}

	// Write compressed data to output stream
	const int written = _outStream.write(compBuf, compressedSize);
	core_free(compBuf);

	if (written != compressedSize) {
		Log::error("Failed to write compressed data");
		return false;
	}

	// Update _pos to track compressed bytes written
	_pos = compressedSize;

	// Clean up
	if (_stream != nullptr) {
		core_free(_stream);
		_stream = nullptr;
	}
	_finalized = true;

	return _outStream.flush();
}

} // namespace io
