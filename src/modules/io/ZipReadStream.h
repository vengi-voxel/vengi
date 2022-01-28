/**
 * @file
 */

#pragma once

#include "Stream.h"

struct mz_stream_s;

namespace io {

/**
 * @see BufferedZipReadStream
 * @see ZipWriteStream
 * @ingroup IO
 */
class ZipReadStream : public io::ReadStream {
private:
	struct mz_stream_s *_stream;
	io::SeekableReadStream &_readStream;
	uint8_t _buf[256 * 1024];
	const int _size;
	int _remaining;
	bool _eos = false;

public:
	ZipReadStream(io::SeekableReadStream &readStream, int size = -1);
	virtual ~ZipReadStream();

	/**
	 * @brief Read an arbitrary sized amount of bytes from the input stream
	 *
	 * @param dataPtr The target data buffer
	 * @param dataSize The size of the target data buffer
	 * @return The amount of read bytes or @c -1 on error
	 */
	int read(void *dataPtr, size_t dataSize) override;
	/**
	 * @return @c true if the end of the compressed stream was found
	 */
	bool eos() const override;
	/**
	 * @brief The remaining amount of bytes to read from the input stream. This is
	 * either the amount of remaining bytes in the input stream, or if the @c size
	 * parameter was specified in the constructor, the amounts of bytes that are left
	 * relative to the size that was specified.
	 *
	 * @return int64_t
	 */
	int64_t remaining() const;
};

} // namespace io