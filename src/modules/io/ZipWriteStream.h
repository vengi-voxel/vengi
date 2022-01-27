/**
 * @file
 */

#pragma once

#include "Stream.h"

struct mz_stream_s;

namespace io {

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
	int64_t pos() const;
};

inline int64_t ZipWriteStream::pos() const {
	return _pos;
}

} // namespace io