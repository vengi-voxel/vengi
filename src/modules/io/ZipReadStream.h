/**
 * @file
 */

#pragma once

#include "Stream.h"
#include "engine-config.h" // USE_ZLIB, USE_LIBDEFLATE

namespace io {

enum class CompressionType : uint8_t {
	Deflate,
	Gzip,
	Zlib
};

/**
 * @see ZipWriteStream
 * @ingroup IO
 */
class ZipReadStream : public io::ReadStream {
private:
	void *_stream;
	io::ReadStream &_readStream;
#ifndef USE_LIBDEFLATE
	uint8_t _buf[256 * 1024] {};
#endif
	const int _size;
	int _remaining;
	int _uncompressedSize = -1;
	bool _eos = false;
	bool _err = false;

public:
	/**
	 * @param size The compressed size
	 */
	ZipReadStream(io::SeekableReadStream &readStream, int size = -1);
	ZipReadStream(io::ReadStream &readStream, int size, CompressionType type);
	virtual ~ZipReadStream();

	static bool isZipStream(io::SeekableReadStream &stream);
	/**
	 * @return The uncompressed size of the zip stream or -1 if not known.
	 */
	int uncompressedSize() const;

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
