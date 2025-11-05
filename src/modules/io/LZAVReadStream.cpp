/**
 * @file
 */

#include "LZAVReadStream.h"
#include "core/Log.h"
#include "core/StandardLib.h"
#include "io/BufferedReadWriteStream.h"

#define LZAV_MALLOC core_malloc
#define LZAV_FREE core_free

#include "external/lzav.h"

namespace io {

bool LZAVReadStream::isLZAVStream(io::SeekableReadStream &readStream) {
	int64_t pos = readStream.pos();

	uint8_t prefix = 0;
	if (readStream.readUInt8(prefix) == -1) {
		readStream.seek(pos);
		return false;
	}

	readStream.seek(pos);
	// LZAV format: upper 4 bits contain format version (1 or 2)
	const int fmt = prefix >> 4;
	return fmt == 1 || fmt == 2;
}

LZAVReadStream::LZAVReadStream(io::SeekableReadStream &readStream, int size)
	: _stream(nullptr), _readStream(readStream), _size(size), _remaining(size >= 0 ? size : 0) {
	// LZAV doesn't have a streaming decompression API
	// We'll decompress on first read to allow proper error handling
	Log::debug("LZAVReadStream created, will decompress on first read");
}

LZAVReadStream::~LZAVReadStream() {
	if (_stream != nullptr) {
		core_free(_stream);
		_stream = nullptr;
	}
}

bool LZAVReadStream::eos() const {
	return _eos;
}

int64_t LZAVReadStream::remaining() const {
	return _remaining;
}

int64_t LZAVReadStream::skip(int64_t delta) {
	const int64_t toSkip = core_min(delta, (int64_t)_remaining);
	_readPos += (int)toSkip;
	_remaining -= (int)toSkip;
	if (_remaining <= 0) {
		_eos = true;
	}
	return toSkip;
}

int LZAVReadStream::read(void *buf, size_t size) {
	if (_err) {
		Log::debug("LZAVReadStream::read() - _err is true");
		return -1;
	}

	// Decompress on first read
	if (_decompressedSize == 0 && !_eos) {
		Log::debug("LZAVReadStream: decompressing stream on first read");

		// Read all compressed data
		BufferedReadWriteStream compressedStream(_readStream, _size <= 0 ? _readStream.remaining() : _size);
		const int64_t compressedSize = compressedStream.size();

		if (compressedSize <= 0) {
			Log::error("Failed to read compressed data");
			_err = true;
			return -1;
		}

		// Allocate decompression buffer - start with 10x compressed size
		size_t decompSize = compressedSize * 10;
		uint8_t *decompBuf = (uint8_t *)core_malloc(decompSize);
		if (decompBuf == nullptr) {
			Log::error("Failed to allocate decompression buffer");
			_err = true;
			return -1;
		}

		// Use lzav_decompress_partial which doesn't require exact output size
		// Try decompression with increasing buffer sizes if needed
		int result = 0;
		while (true) {
			result = lzav_decompress_partial(compressedStream.getBuffer(), decompBuf, (int)compressedSize,
											 (int)decompSize);

			if (result > 0) {
				// Success
				break;
			} else {
				// Need larger buffer
				decompSize *= 2;
				decompBuf = (uint8_t *)core_realloc(decompBuf, decompSize);
				if (decompBuf == nullptr) {
					Log::error("Failed to reallocate decompression buffer");
					_err = true;
					return -1;
				}
			}
		}

		// Store decompressed data in internal buffer
		const size_t actualSize = (size_t)result;
		if (actualSize <= sizeof(_buf)) {
			core_memcpy(_buf, decompBuf, actualSize);
			core_free(decompBuf);
			_stream = nullptr;
		} else {
			// Data is larger than our buffer, keep the allocated buffer
			_stream = decompBuf;
		}

		_decompressedSize = (int)actualSize;
		_remaining = (int)actualSize;
		_readPos = 0;

		Log::debug("LZAVReadStream: decompressed %d bytes from %ld compressed bytes", _decompressedSize,
				   compressedSize);
	}

	if (_eos || _remaining <= 0) {
		return 0;
	}

	const size_t toRead = core_min(size, (size_t)_remaining);
	const uint8_t *srcData = _stream != nullptr ? (const uint8_t *)_stream : _buf;

	core_memcpy(buf, srcData + _readPos, toRead);
	_readPos += (int)toRead;
	_remaining -= (int)toRead;

	if (_remaining <= 0) {
		_eos = true;
	}

	return (int)toRead;
}

} // namespace io
