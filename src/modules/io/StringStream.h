/**
 * @file
 */

#pragma once

#include "core/String.h"
#include "io/Stream.h"

namespace io {

/**
 * @ingroup IO
 * @see SeekableReadStream
 */
class StringReadStream : public SeekableReadStream {
protected:
	SeekableReadStream &_stream;
public:
	StringReadStream(SeekableReadStream &stream);
	~StringReadStream();

	core::String readAll();

	int64_t size() const override;
	int64_t pos() const override;
	int read(void *dataPtr, size_t dataSize) override;
	int64_t seek(int64_t position, int whence = SEEK_SET) override;
};

} // namespace io
