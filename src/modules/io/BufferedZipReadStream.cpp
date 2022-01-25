/**
 * @file
 */

#include "BufferedZipReadStream.h"
#include "core/Log.h"
#include "core/StandardLib.h"
#include "core/Zip.h"
#include "io/MemoryReadStream.h"

namespace io {

BufferedZipReadStream::BufferedZipReadStream(SeekableReadStream &stream, size_t zipSize, size_t maxUncompressedSize)
	: MemoryReadStream(nullptr, 0) {
	core_assert(maxUncompressedSize > 0);

	uint8_t *srcBuf = (uint8_t *)core_malloc(zipSize);
	if (stream.read(srcBuf, zipSize) != 0) {
		// failed - make sure that the next read from this stream fails, too
		_size = 0;
		_pos = 0;
		Log::debug("Failed to read %i bytes from parent stream", (int)zipSize);
		core_free(srcBuf);
		return;
	}

	uint8_t *buf = (uint8_t *)core_malloc(maxUncompressedSize * 2);
	size_t finalSize = 0;
	if (!core::zip::uncompress(srcBuf, zipSize, buf, maxUncompressedSize * 2, &finalSize)) {
		// failed - make sure that the next read from this stream fails, too
		_size = 0;
		_pos = 0;
		core_free(buf);
		Log::error("Failed to uncompress stream data");
	} else {
		_buf = buf;
		_size = (int64_t)finalSize;
	}

	core_free(srcBuf);
}

BufferedZipReadStream::~BufferedZipReadStream() {
	core_free((uint8_t *)_buf);
}

} // namespace io