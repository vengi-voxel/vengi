/**
 * @file
 */

#pragma once

#include "Stream.h"

struct mz_stream_s;

namespace io {

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

	int read(void *dataPtr, size_t dataSize) override;
	bool eos() const override;
	int64_t remaining() const;
};

} // namespace io