/**
 * @file
 */

#pragma once

#include "engine-config.h" // USE_LZ4

#ifdef USE_LZ4

#include "Stream.h"

namespace io {

/**
 * @see LZ4ReadStream
 * @see WriteStream
 * @ingroup IO
 */
class LZ4WriteStream : public io::WriteStream {
private:
	void *_stream;
	io::WriteStream &_outStream;
	uint8_t _out[256 * 1024]{};
	int64_t _pos = 0;
	bool _finalized = false;

public:
	/**
	 * @param outStream The buffer that receives the writes for the compressed data.
	 * @param level The compression level (0 = fast compression, 1-9 = HC compression with increasing quality).
	 */
	LZ4WriteStream(io::WriteStream &outStream, int level = 6);
	virtual ~LZ4WriteStream();

	/**
	 * @param buf The buffer to add to the output stream.
	 * @param size The size of the buffer.
	 * @return @c -1 on error - otherwise the amount of bytes that were written to the output stream.
	 * The amount of bytes written are usually less than the given input buffer size, as the bytes
	 * that are written to the output buffer are compressed already.
	 */
	int write(const void *buf, size_t size) override;
	/**
	 * @brief Returns the compressed written bytes that went into the given output stream
	 */
	int64_t pos() const;
	/**
	 * @brief Returns the compressed written bytes that went into the given output stream
	 */
	int64_t size() const;

	/**
	 * @brief Flush the pending stream data into the output stream
	 *
	 * @note This method is automatically called in the destructor
	 */
	bool flush() override;
};

inline int64_t LZ4WriteStream::pos() const {
	return _pos;
}

inline int64_t LZ4WriteStream::size() const {
	return _pos;
}

} // namespace io

#endif // USE_LZ4
