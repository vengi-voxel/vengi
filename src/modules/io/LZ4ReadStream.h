/**
 * @file
 */

#pragma once

#include "Stream.h"
#include "io/MemoryReadStream.h"

struct LZ4F_dctx_s;

namespace io {

/** Reads one LZ4 frame directly from a compressed buffer owned by the caller. */
class LZ4FrameReader : public core::NonCopyable {
private:
	LZ4F_dctx_s *_ctx = nullptr;
	const uint8_t *_input;
	size_t _remaining;
	bool _finished = false;
	bool _failed = false;

public:
	LZ4FrameReader(const uint8_t *data, size_t size);
	~LZ4FrameReader();

	/** Returns the bytes read, zero at frame end, or -1 on error. */
	int read(void *buf, size_t size);
	bool skip(size_t size);
	bool eos() const {
		return _finished;
	}
};

/**
 * @brief Seekable wrapper around an LZ4 frame compressed stream
 * @ingroup IO
 */
class LZ4ReadStream : public io::SeekableReadStream {
private:
	io::MemoryReadStream *_readStream = nullptr;
	uint8_t *_extractedBuffer = nullptr;

public:
	/**
	 * @param size The compressed size; -1 reads remaining bytes from @p readStream
	 */
	LZ4ReadStream(io::SeekableReadStream &readStream, int size = -1);
	virtual ~LZ4ReadStream();

	int read(void *dataPtr, size_t dataSize) override;
	int64_t seek(int64_t position, int whence = SEEK_SET) override;
	int64_t size() const override;
	int64_t pos() const override;
};

} // namespace io
