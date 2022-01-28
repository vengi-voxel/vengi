/**
 * @file
 */

#pragma once

#include "Stream.h"

struct mz_stream_s;

namespace io {

/**
 * @see BufferedZipReadStream
 * @see ZipReadStream
 */
class ZipWriteStream : public io::WriteStream {
private:
	struct mz_stream_s *_stream;
	io::WriteStream &_outStream;
	uint8_t _out[256 * 1024];
	int64_t _pos = 0;

public:
	ZipWriteStream(io::WriteStream &out, int level = 6);
	virtual ~ZipWriteStream();

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
	bool flush();
};

inline int64_t ZipWriteStream::pos() const {
	return _pos;
}

inline int64_t ZipWriteStream::size() const {
	return _pos;
}

} // namespace io