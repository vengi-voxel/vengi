/**
 * @file
 */

#pragma once

#include "Stream.h"

namespace io {

/**
 * @see LZAVWriteStream
 * @ingroup IO
 */
class LZAVReadStream : public io::ReadStream {
private:
	void *_stream;
	io::SeekableReadStream &_readStream;
	uint8_t _buf[256 * 1024]{};
	const int _size;
	int _remaining;
	int _decompressedSize = 0;
	int _readPos = 0;
	bool _eos = false;
	bool _err = false;

public:
	/**
	 * @param size The compressed size
	 */
	LZAVReadStream(io::SeekableReadStream &readStream, int size = -1);
	virtual ~LZAVReadStream();

	static bool isLZAVStream(io::SeekableReadStream &stream);

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

	bool err() const {
		return _err;
	}

	/**
	 * @brief Advances the position in the stream without reading the bytes.
	 * @param delta the bytes to skip
	 * @return -1 on error
	 */
	int64_t skip(int64_t delta);

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
