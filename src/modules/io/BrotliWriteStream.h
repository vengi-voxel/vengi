/**
 * @file
 */

#pragma once

#include "Stream.h"

namespace io {

/**
 * @brief WriteStream wrapper that Brotli-compresses into another stream
 * @see BrotliReadStream
 * @ingroup IO
 */
class BrotliWriteStream : public io::WriteStream {
private:
	void *_state = nullptr;
	io::WriteStream &_outStream;
	uint8_t _out[64 * 1024]{};
	int64_t _pos = 0;
	bool _finished = false;

	bool compress(const uint8_t *data, size_t size, int operation);

public:
	/**
	 * @param outStream Destination for compressed bytes
	 * @param quality Brotli quality 0-11. VoxelCdx uses .NET CompressionLevel.Optimal (4).
	 */
	explicit BrotliWriteStream(io::WriteStream &outStream, int quality = 4);
	virtual ~BrotliWriteStream();

	int write(const void *buf, size_t size) override;
	bool flush() override;

	int64_t pos() const {
		return _pos;
	}
	int64_t size() const {
		return _pos;
	}
};

} // namespace io
