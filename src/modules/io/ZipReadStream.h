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
	io::ReadStream &_readStream;
	uint8_t _buf[256 * 1024];
	bool _eos = false;

public:
	ZipReadStream(io::ReadStream &readStream);
	virtual ~ZipReadStream();

	int read(void *dataPtr, size_t dataSize) override;
	bool eos() const override;
};

} // namespace io