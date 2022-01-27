/**
 * @file
 */

#include "ZipWriteStream.h"
#include "core/StandardLib.h"
#include "core/miniz.h"

namespace io {

ZipWriteStream::ZipWriteStream(io::WriteStream &out, int level) : _outStream(out) {
	_stream = (mz_stream *)core_malloc(sizeof(*_stream));
	core_memset(_stream, 0, sizeof(*_stream));
	mz_deflateInit(_stream, level);
}

ZipWriteStream::~ZipWriteStream() {
	const int retVal = mz_deflateEnd(_stream);
	core_free(_stream);
	core_assert(retVal == MZ_OK);
}

int ZipWriteStream::write(const void *buf, size_t size) {
	_stream->next_in = (unsigned char *)buf;
	_stream->avail_in = static_cast<unsigned int>(size);

	while (_stream->avail_in > 0) {
		_stream->avail_out = sizeof(_out);
		_stream->next_out = _out;

		const int retVal = mz_deflate(_stream, MZ_NO_FLUSH);
		if (retVal != MZ_OK) {
			return -1;
		}

		const uint32_t written = sizeof(_out) - _stream->avail_out;
		if (written != 0u) {
			if (_outStream.write(_out, written) == -1) {
				return -1;
			}
		}
	}
	_pos += (int64_t)size;
	return (int)size;
}

} // namespace io