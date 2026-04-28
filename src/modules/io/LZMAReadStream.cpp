/**
 * @file
 */

#include "LZMAReadStream.h"
#include "core/Log.h"
#include "core/StandardLib.h"
#include "io/BufferedReadWriteStream.h"

extern "C" {
#include "minlzlib.h"
}

namespace io {

LZMAReadStream::LZMAReadStream(io::SeekableReadStream &readStream, int size) {
	BufferedReadWriteStream s(readStream, size <= 0 ? readStream.remaining() : size);

	// First pass: get decompressed size
	BfInitialize(s.getBuffer(), (uint32_t)s.size());
	DtInitialize(nullptr, 0, 0);
	uint32_t decompressedSize = 0;
	if (!Lz2DecodeStream(&decompressedSize, true)) {
		Log::error("LZMA: failed to determine decompressed size");
		return;
	}

	// Second pass: decompress
	_extractedBuffer = (uint8_t *)core_malloc(decompressedSize);
	BfInitialize(s.getBuffer(), (uint32_t)s.size());
	DtInitialize(_extractedBuffer, decompressedSize, 0);
	uint32_t bytesProcessed = 0;
	if (!Lz2DecodeStream(&bytesProcessed, false)) {
		Log::error("LZMA: decompression failed");
		core_free(_extractedBuffer);
		_extractedBuffer = nullptr;
		return;
	}
	_readStream = new MemoryReadStream(_extractedBuffer, decompressedSize);
}

LZMAReadStream::~LZMAReadStream() {
	delete _readStream;
	core_free(_extractedBuffer);
}

int64_t LZMAReadStream::seek(int64_t position, int whence) {
	if (_readStream == nullptr) {
		return -1;
	}
	return _readStream->seek(position, whence);
}

int64_t LZMAReadStream::size() const {
	if (_readStream == nullptr) {
		return 0;
	}
	return _readStream->size();
}

int64_t LZMAReadStream::pos() const {
	if (_readStream == nullptr) {
		return 0;
	}
	return _readStream->pos();
}

int LZMAReadStream::read(void *buf, size_t size) {
	if (_readStream == nullptr) {
		return -1;
	}
	return _readStream->read(buf, size);
}

} // namespace io
