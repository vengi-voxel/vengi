/**
 * @file
 */

#include "ZipWriteStream.h"
#include "core/StandardLib.h"
#include "engine-config.h" // USE_ZLIB
#if USE_ZLIB
#define ZLIB_CONST
#ifndef Z_DEFAULT_WINDOW_BITS
#define Z_DEFAULT_WINDOW_BITS 15
#endif
#include <zlib.h>
#else
#include "io/external/miniz.h"
#endif
#include "core/Assert.h"

namespace io {

ZipWriteStream::ZipWriteStream(io::WriteStream &outStream, int level, bool rawDeflate) : _outStream(outStream) {
	_stream = (z_stream *)core_malloc(sizeof(z_stream));
	core_memset(((z_stream*)_stream), 0, sizeof(*((z_stream*)_stream)));
	((z_stream*)_stream)->zalloc = Z_NULL;
	((z_stream*)_stream)->zfree = Z_NULL;
	const int windowBits = rawDeflate ? -Z_DEFAULT_WINDOW_BITS : Z_DEFAULT_WINDOW_BITS;
	deflateInit2(((z_stream *)_stream), level, Z_DEFLATED, windowBits, 9, Z_DEFAULT_STRATEGY);
}

ZipWriteStream::~ZipWriteStream() {
	ZipWriteStream::flush();
	const int retVal = deflateEnd(((z_stream*)_stream));
	core_free(((z_stream*)_stream));
	core_assert(retVal == Z_OK);
	(void)retVal;
}

int ZipWriteStream::write(const void *buf, size_t size) {
	((z_stream*)_stream)->next_in = (const unsigned char *)buf;
	((z_stream*)_stream)->avail_in = static_cast<unsigned int>(size);

	uint32_t writtenBytes = 0;
	while (((z_stream*)_stream)->avail_in > 0) {
		((z_stream*)_stream)->avail_out = sizeof(_out);
		((z_stream*)_stream)->next_out = _out;

		const int retVal = deflate(((z_stream*)_stream), Z_NO_FLUSH);
		if (retVal != Z_OK) {
			return -1;
		}

		const uint32_t written = sizeof(_out) - ((z_stream*)_stream)->avail_out;
		if (written != 0u) {
			if (_outStream.write(_out, written) != (int)written) {
				return -1;
			}
			writtenBytes += written;
			_pos += written;
		}
	}
	return (int)writtenBytes;
}

bool ZipWriteStream::flush() {
	((z_stream*)_stream)->avail_in = 0;
	((z_stream*)_stream)->avail_out = sizeof(_out);
	((z_stream*)_stream)->next_out = _out;

	const int retVal = deflate(((z_stream*)_stream), Z_FINISH);
	if (retVal == Z_STREAM_ERROR) {
		return false;
	}

	size_t remaining = sizeof(_out) - ((z_stream*)_stream)->avail_out;
	const int written = _outStream.write(_out, remaining);
	_pos += written;
	return written == (int)remaining;
}

} // namespace io
