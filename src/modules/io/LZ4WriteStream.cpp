/**
 * @file
 */

#include "LZ4WriteStream.h"
#include "core/Log.h"
#include "core/StandardLib.h"

#include <lz4frame.h>

namespace io {

LZ4WriteStream::LZ4WriteStream(io::WriteStream &outStream, int compressionLevel)
	: _outStream(outStream), _compressionLevel(compressionLevel) {
	LZ4F_cctx *cctx = nullptr;
	const LZ4F_errorCode_t err = LZ4F_createCompressionContext(&cctx, LZ4F_VERSION);
	if (LZ4F_isError(err) || cctx == nullptr) {
		Log::error("LZ4: failed to create compression context: %s", LZ4F_getErrorName(err));
		return;
	}
	_cctx = cctx;
}

LZ4WriteStream::~LZ4WriteStream() {
	LZ4WriteStream::flush();
	if (_cctx != nullptr) {
		LZ4F_freeCompressionContext((LZ4F_cctx *)_cctx);
		_cctx = nullptr;
	}
	core_free(_out);
	_out = nullptr;
}

bool LZ4WriteStream::ensureOutCapacity(size_t needed) {
	if (needed <= _outCapacity) {
		return true;
	}
	uint8_t *resized = (uint8_t *)core_realloc(_out, needed);
	if (resized == nullptr) {
		Log::error("LZ4: failed to allocate output buffer");
		return false;
	}
	_out = resized;
	_outCapacity = needed;
	return true;
}

bool LZ4WriteStream::writeCompressed(const void *src, size_t srcSize, bool end) {
	if (_cctx == nullptr) {
		return false;
	}
	LZ4F_cctx *cctx = (LZ4F_cctx *)_cctx;
	LZ4F_preferences_t prefs;
	core_memset(&prefs, 0, sizeof(prefs));
	prefs.frameInfo.blockSizeID = LZ4F_max64KB;
	prefs.frameInfo.blockMode = LZ4F_blockLinked;
	prefs.frameInfo.contentChecksumFlag = LZ4F_contentChecksumEnabled;
	prefs.compressionLevel = _compressionLevel;

	if (!_begun) {
		if (!ensureOutCapacity(LZ4F_HEADER_SIZE_MAX)) {
			return false;
		}
		const size_t headerSize = LZ4F_compressBegin(cctx, _out, _outCapacity, &prefs);
		if (LZ4F_isError(headerSize)) {
			Log::error("LZ4: compressBegin failed: %s", LZ4F_getErrorName(headerSize));
			return false;
		}
		if (headerSize > 0) {
			if (_outStream.write(_out, headerSize) != (int)headerSize) {
				Log::error("LZ4: failed to write frame header");
				return false;
			}
			_pos += (int64_t)headerSize;
		}
		_begun = true;
	}

	if (srcSize > 0) {
		const size_t bound = LZ4F_compressBound(srcSize, &prefs);
		if (!ensureOutCapacity(bound)) {
			return false;
		}
		const size_t written =
			LZ4F_compressUpdate(cctx, _out, _outCapacity, src, srcSize, nullptr);
		if (LZ4F_isError(written)) {
			Log::error("LZ4: compressUpdate failed: %s", LZ4F_getErrorName(written));
			return false;
		}
		if (written > 0) {
			if (_outStream.write(_out, written) != (int)written) {
				Log::error("LZ4: failed to write compressed data");
				return false;
			}
			_pos += (int64_t)written;
		}
	}

	if (end) {
		const size_t bound = LZ4F_compressBound(0, &prefs);
		if (!ensureOutCapacity(bound)) {
			return false;
		}
		const size_t written = LZ4F_compressEnd(cctx, _out, _outCapacity, nullptr);
		if (LZ4F_isError(written)) {
			Log::error("LZ4: compressEnd failed: %s", LZ4F_getErrorName(written));
			return false;
		}
		if (written > 0) {
			if (_outStream.write(_out, written) != (int)written) {
				Log::error("LZ4: failed to write frame footer");
				return false;
			}
			_pos += (int64_t)written;
		}
	}
	return true;
}

int LZ4WriteStream::write(const void *buf, size_t size) {
	if (_finished) {
		return -1;
	}
	if (size == 0) {
		return 0;
	}
	if (!writeCompressed(buf, size, false)) {
		return -1;
	}
	return (int)size;
}

bool LZ4WriteStream::flush() {
	if (_finished) {
		return true;
	}
	_finished = true;
	return writeCompressed(nullptr, 0, true);
}

} // namespace io
