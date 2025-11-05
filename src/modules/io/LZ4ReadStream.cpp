/**
 * @file
 */

#include "LZ4ReadStream.h"

#ifdef USE_LZ4

#include "core/Assert.h"
#include "core/Log.h"
#include <lz4.h>
#include <lz4frame.h>

namespace io {

bool LZ4ReadStream::isLZ4Stream(io::SeekableReadStream &readStream) {
	int64_t pos = readStream.pos();

	uint32_t magic = 0;
	if (readStream.readUInt32(magic) == -1) {
		readStream.seek(pos);
		return false;
	}

	readStream.seek(pos);
	return magic == 0x184D2204;
}

LZ4ReadStream::LZ4ReadStream(io::SeekableReadStream &readStream, int size)
	: _readStream(readStream), _size(size), _remaining(size), _srcSize(0), _srcOffset(0), _headerRead(false) {
	// Initialize LZ4 decompression context
	LZ4F_dctx *dctx = nullptr;
	LZ4F_errorCode_t result = LZ4F_createDecompressionContext(&dctx, LZ4F_VERSION);
	if (LZ4F_isError(result)) {
		Log::error("Failed to create LZ4 decompression context: %s", LZ4F_getErrorName(result));
		_err = true;
		_stream = nullptr;
	} else {
		Log::debug("LZ4ReadStream created successfully");
		_stream = dctx;
	}
}

LZ4ReadStream::~LZ4ReadStream() {
	if (_stream != nullptr) {
		LZ4F_freeDecompressionContext((LZ4F_dctx *)_stream);
		_stream = nullptr;
	}
}

bool LZ4ReadStream::eos() const {
	return _eos;
}

int64_t LZ4ReadStream::remaining() const {
	if (_size >= 0) {
		core_assert_msg(_remaining >= 0, "if size is given (%i), remaining should be >= 0 - but is %i", _size,
						_remaining);
		return core_min(_remaining, _readStream.remaining());
	}
	return _readStream.remaining();
}

int64_t LZ4ReadStream::skip(int64_t delta) {
	int64_t bytesSkipped = 0;
	uint8_t tempBuffer[1024];
	while (bytesSkipped < delta) {
		int64_t chunk = core_min(delta - bytesSkipped, (int64_t)sizeof(tempBuffer));
		if (read(tempBuffer, chunk) < chunk) {
			_err = true;
			return -1;
		}
		bytesSkipped += chunk;
	}
	return bytesSkipped;
}

int LZ4ReadStream::read(void *buf, size_t size) {
	if (_err) {
		Log::debug("LZ4ReadStream::read() - _err is true");
		return -1;
	}
	if (_stream == nullptr) {
		Log::debug("LZ4ReadStream::read() - _stream is nullptr");
		return -1;
	}
	if (_eos) {
		return 0;
	}

	LZ4F_dctx *dctx = (LZ4F_dctx *)_stream;
	uint8_t *targetPtr = (uint8_t *)buf;
	size_t totalDecompressed = 0;

	while (totalDecompressed < size) {
		if (_srcOffset >= _srcSize) {
			int64_t remainingSize = remaining();
			if (remainingSize <= 0) {
				if (!_headerRead) {
					Log::debug("No data available to read LZ4 header");
					_err = true;
					return -1;
				}
				if (totalDecompressed > 0) {
					return (int)totalDecompressed;
				}
				_eos = true;
				return 0;
			}

			const size_t readSize = core_min((int64_t)sizeof(_buf), remainingSize);
			const int bytesRead = _readStream.read(_buf, readSize);
			if (bytesRead <= 0) {
				if (bytesRead == -1) {
					Log::debug("Failed to read from parent stream");
					_err = true;
					return -1;
				}
				if (!_headerRead) {
					Log::debug("Parent stream at EOF before LZ4 header could be read");
					_err = true;
					return -1;
				}
				if (totalDecompressed > 0) {
					return (int)totalDecompressed;
				}
				_eos = true;
				return 0;
			}

			if (_size >= 0) {
				_remaining -= bytesRead;
			}

			_srcSize = bytesRead;
			_srcOffset = 0;
		}

		size_t dstSize = size - totalDecompressed;
		size_t srcSizeToConsume = _srcSize - _srcOffset;

		LZ4F_errorCode_t result = LZ4F_decompress(dctx, targetPtr + totalDecompressed, &dstSize, _buf + _srcOffset,
												  &srcSizeToConsume, nullptr);

		if (LZ4F_isError(result)) {
			Log::error("LZ4 decompression error: %s", LZ4F_getErrorName(result));
			_err = true;
			return -1;
		}

		_srcOffset += srcSizeToConsume;
		totalDecompressed += dstSize;

		if (srcSizeToConsume > 0 || dstSize > 0) {
			_headerRead = true;
		}

		if (result == 0) {
			_eos = true;
			break;
		}

		// If we didn't make any progress AND we've consumed all available source data,
		// we need to read more source data before trying again
		if (dstSize == 0 && srcSizeToConsume == 0) {
			if (_srcOffset >= _srcSize) {
				continue;
			}
			Log::error("LZ4 decompression stalled");
			_err = true;
			return -1;
		}
	}

	return (int)totalDecompressed;
}

} // namespace io

#endif // USE_LZ4
