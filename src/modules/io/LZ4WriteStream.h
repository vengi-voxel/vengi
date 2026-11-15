/**
 * @file
 */

#pragma once

#include "Stream.h"

namespace io {

/**
 * @brief WriteStream wrapper that LZ4-frame-compresses into another stream
 * @see LZ4ReadStream
 * @ingroup IO
 */
class LZ4WriteStream : public io::WriteStream {
private:
	void *_cctx = nullptr;
	io::WriteStream &_outStream;
	uint8_t *_out = nullptr;
	size_t _outCapacity = 0;
	int64_t _pos = 0;
	bool _finished = false;
	bool _begun = false;
	int _compressionLevel = 0;

	bool ensureOutCapacity(size_t needed);
	bool writeCompressed(const void *src, size_t srcSize, bool end);

public:
	/**
	 * @param outStream Destination for compressed bytes (LZ4 frame format)
	 * @param compressionLevel 0 = default fast; >0 uses HC up to max; <0 is fast acceleration
	 */
	explicit LZ4WriteStream(io::WriteStream &outStream, int compressionLevel = 0);
	virtual ~LZ4WriteStream();

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
