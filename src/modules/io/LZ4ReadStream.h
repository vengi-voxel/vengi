/**
 * @file
 */

#pragma once

#include "engine-config.h" // USE_LZ4

#ifdef USE_LZ4

#include "Stream.h"

namespace io {

/**
 * @see LZ4WriteStream
 * @ingroup IO
 */
class LZ4ReadStream : public io::ReadStream {
private:
	void *_stream;
	io::SeekableReadStream &_readStream;
	uint8_t _buf[256 * 1024]{};
	const int _size;
	int _remaining;
	size_t _srcSize = 0;	  // Amount of valid data in _buf
	size_t _srcOffset = 0;	  // Current read position in _buf
	bool _headerRead = false; // Whether we've successfully read the LZ4 frame header
	bool _eos = false;
	bool _err = false;

public:
	/**
	 * @param size The compressed size
	 */
	LZ4ReadStream(io::SeekableReadStream &readStream, int size = -1);
	virtual ~LZ4ReadStream();

	static bool isLZ4Stream(io::SeekableReadStream &stream);

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

#endif // USE_LZ4
