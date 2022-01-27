/**
 * @file
 */

#include "ZipReadStream.h"
#include "core/StandardLib.h"
#include "core/miniz.h"

namespace io {

ZipReadStream::ZipReadStream(io::ReadStream &readStream) : _readStream(readStream) {
	_stream = (mz_stream *)core_malloc(sizeof(*_stream));
	core_memset(_stream, 0, sizeof(*_stream));
	mz_inflateInit(_stream);
}

ZipReadStream::~ZipReadStream() {
	mz_inflateEnd(_stream);
	core_free(_stream);
}

bool ZipReadStream::eos() const {
	return _eos;
}

int ZipReadStream::read(void *buf, size_t size) {
	_stream->next_out = (unsigned char *)buf;
	_stream->avail_out = static_cast<unsigned int>(size);

	while (_stream->avail_out != 0) {
		if (_stream->avail_in == 0) {
			const int retVal = _readStream.read(_buf, sizeof(_buf));
			if (retVal == -1) {
				return -1;
			}
			_stream->next_in = _buf;
			_stream->avail_in = static_cast<unsigned int>(retVal);
		}

		const int mzRet = mz_inflate(_stream, MZ_NO_FLUSH);
		if (mzRet == MZ_STREAM_END) {
			_eos = true;
			return (int)(size - _stream->avail_out);
		}

		if (mzRet != MZ_OK) {
			return -1;
		}
	}

	return (int)(size - _stream->avail_out);
}

} // namespace io