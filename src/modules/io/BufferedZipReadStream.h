/**
 * @file
 */

#pragma once

#include "io/MemoryReadStream.h"

namespace io {

class BufferedZipReadStream : public MemoryReadStream {
public:
	BufferedZipReadStream(SeekableReadStream &stream, size_t zipSize, size_t maxUncompressedSize);

	~BufferedZipReadStream();
};

} // namespace io
