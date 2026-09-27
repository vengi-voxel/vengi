/**
 * @file
 */

#pragma once

#include "Stream.h"
#include "io/MemoryReadStream.h"

namespace io {

/**
 * @brief Seekable wrapper around a Brotli compressed stream
 * @ingroup IO
 */
class BrotliReadStream : public io::SeekableReadStream {
private:
	io::MemoryReadStream *_readStream = nullptr;
	uint8_t *_extractedBuffer = nullptr;

public:
	/**
	 * @param size The compressed size
	 */
	BrotliReadStream(io::SeekableReadStream &readStream, int size = -1);
	virtual ~BrotliReadStream();

	int read(void *dataPtr, size_t dataSize) override;
	int64_t seek(int64_t position, int whence = SEEK_SET) override;
	int64_t size() const override;
	int64_t pos() const override;
};

} // namespace io
