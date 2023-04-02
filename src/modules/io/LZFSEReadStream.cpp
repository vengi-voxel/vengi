/**
 * @file
 */

#include "LZFSEReadStream.h"
#include "core/Assert.h"
#include "core/Log.h"
#include "core/StandardLib.h"
#include "core/collection/Buffer.h"
#include "io/BufferedReadWriteStream.h"
#include "lzfse.h"

namespace io {

LZFSEReadStream::LZFSEReadStream(io::SeekableReadStream &readStream, int size) {
	// there is no streaming interface in lzfse, so we need to read the whole stream into memory
	BufferedReadWriteStream s(readStream, size <= 0 ? readStream.remaining() : size);
	size_t extractedBufferSize = 10 * s.size();
	void *auxBuffer = core_malloc(lzfse_decode_scratch_size());
	_extractedBuffer = (uint8_t *)malloc(extractedBufferSize);
	size_t extractedSize = 0;
	while (1) {
		extractedSize = lzfse_decode_buffer(_extractedBuffer, extractedBufferSize, s.getBuffer(), s.size(), auxBuffer);
		if (extractedSize == 0 || extractedSize == extractedBufferSize) {
			extractedBufferSize <<= 1;
			_extractedBuffer = (uint8_t *)core_realloc(_extractedBuffer, extractedBufferSize);
			continue;
		}
		break;
	}
	_readStream = new MemoryReadStream(_extractedBuffer, extractedSize);
	core_free(auxBuffer);
}

LZFSEReadStream::~LZFSEReadStream() {
	delete _readStream;
	core_free(_extractedBuffer);
}

bool LZFSEReadStream::eos() const {
	return _readStream->eos();
}

int64_t LZFSEReadStream::remaining() const {
	return _readStream->remaining();
}

int64_t LZFSEReadStream::skip(int64_t delta) {
	return _readStream->skip(delta);
}

int LZFSEReadStream::read(void *buf, size_t size) {
	return _readStream->read(buf, size);
}

} // namespace io
