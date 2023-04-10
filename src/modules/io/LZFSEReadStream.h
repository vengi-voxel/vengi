/**
 * @file
 */

#pragma once

#include "Stream.h"
#include "io/MemoryReadStream.h"

namespace io {

/**
 * @ingroup IO
 */
class LZFSEReadStream : public io::SeekableReadStream {
private:
	io::MemoryReadStream *_readStream = nullptr;
	uint8_t *_extractedBuffer = nullptr;

public:
	/**
	 * @param size The compressed size
	 */
	LZFSEReadStream(io::SeekableReadStream &readStream, int size = -1);
	virtual ~LZFSEReadStream();

	/**
	 * @brief Read an arbitrary sized amount of bytes from the input stream
	 *
	 * @param dataPtr The target data buffer
	 * @param dataSize The size of the target data buffer
	 * @return The amount of read bytes or @c -1 on error
	 */
	int read(void *dataPtr, size_t dataSize) override;
	/**
	 * @param[in] position This is the number of bytes to offset
	 * @param[in] whence @c SEEK_SET offset is used as absolute position from the beginning of the stream.
	 * @c SEEK_CUR offset is taken as relative offset from the current position.
	 * @c SEEK_END offset is used relative to the end of the stream.
	 * @return -1 on error - otherwise the current offset in the stream
	 */
	int64_t seek(int64_t position, int whence = SEEK_SET) override;
	int64_t size() const override;
	int64_t pos() const override;
};

} // namespace io
