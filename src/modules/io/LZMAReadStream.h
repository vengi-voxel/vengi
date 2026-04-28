/**
 * @file
 */

#pragma once

#include "Stream.h"
#include "io/MemoryReadStream.h"

namespace io {

/**
 * @brief Read stream for raw LZMA2 compressed data (as used by Python lzma.FORMAT_RAW with FILTER_LZMA2)
 * @ingroup IO
 */
class LZMAReadStream : public io::SeekableReadStream {
private:
	io::MemoryReadStream *_readStream = nullptr;
	uint8_t *_extractedBuffer = nullptr;

public:
	/**
	 * @param readStream The stream containing the compressed data
	 * @param size The compressed size (-1 to read until end of stream)
	 */
	LZMAReadStream(io::SeekableReadStream &readStream, int size = -1);
	virtual ~LZMAReadStream();

	int read(void *dataPtr, size_t dataSize) override;
	int64_t seek(int64_t position, int whence = SEEK_SET) override;
	int64_t size() const override;
	int64_t pos() const override;
};

} // namespace io
